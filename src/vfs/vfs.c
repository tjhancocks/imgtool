/*
  Copyright (c) 2017 Tom Hancocks
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <vfs/vfs.h>
#include <vfs/interface.h>

vfs_t vfs_init(vdevice_t dev, vfs_interface_t interface)
{
    assert(dev);
    assert(interface);

    vfs_t vfs = calloc(1, sizeof(*vfs));
    vfs->device = dev;
    vfs->filesystem_interface = interface;
    vfs->type = interface->type_name();

    return vfs;
}

void vfs_destroy(vfs_t vfs)
{
    if (vfs) {
        vfs->filesystem_interface->unmount_filesystem(vfs);
        vfs_interface_destroy(vfs->filesystem_interface);
        device_destroy(vfs->device);
    }
    free(vfs);
}

vfs_t vfs_mount(vdevice_t dev)
{
    if (!dev) {
        return NULL;
    }

    // Create an interface for the device. If NULL then we do not have a
    // readable device attached.
    vfs_interface_t vfsi = vfs_interface_for_device(dev);
    if (!vfsi) {
        return NULL;
    }

    // Setup the VFS.
    vfs_t vfs = vfs_init(dev, vfsi);

    // Now mount the file system, and ensure the current directory is root.
    vfs->assoc_info = vfsi->mount_filesystem(vfs);
    vfsi->set_directory(vfs, NULL);

    return vfs;
}

vfs_t vfs_unmount(vfs_t vfs)
{
    if (vfs) {
        vfs->filesystem_interface->unmount_filesystem(vfs);
    }
    return NULL;
}


const char *vfs_pwd(vfs_t vfs)
{
    if (vfs && vfs->assoc_info) {
        return "/";
    }
    else {
        return "<unmounted>";
    }
}

vfs_node_t vfs_get_directory_list(vfs_t vfs)
{
    assert(vfs);
    if (vfs->assoc_info) {
        return vfs->filesystem_interface->get_directory_list(vfs);
    }
    return NULL;
}

void vfs_navigate_to_path(vfs_t vfs, const char *path)
{
    assert(vfs);
    vfs_path_node_t path_node = vfs_construct_path(path);

    // Get the original directory
    vfs_node_t orig_dir = vfs->filesystem_interface->current_directory(vfs);

    if (!path_node || (path_node && path_node->is_root)) {
        // Navigate to the root node.
        vfs->filesystem_interface->set_directory(vfs, NULL);
    }

    if (!path_node || strlen(path_node->name) == 0) {
        // We have nothing left to do. Finish.
        return;
    }

    while (path_node) {
        // Search for a node in the current directory with the name of the
        // path node. Fail if we don't and restore the original directory.
        const char *name = path_node->name;
        vfs_node_t dir = vfs->filesystem_interface->get_node(vfs, name);

        if (!dir || !(dir && dir->attributes & vfs_node_directory_attribute)) {
            // Failure. Navigate back to the original directory
            vfs->filesystem_interface->set_directory(vfs, orig_dir);
            break;
        }

        // Set the directory to this one, and continue descending.
        vfs->filesystem_interface->set_directory(vfs, dir);

        // Next path component
        path_node = path_node->next;
    }
    
    // Clean up
    vfs_node_destroy(orig_dir);
}

void vfs_touch(vfs_t vfs, const char *path)
{
    assert(vfs);
    vfs->filesystem_interface->create_file(vfs, path, 0);
}

int vfs_mkdir(vfs_t vfs, const char *path)
{
    assert(vfs);
    
    // Get the current directory. We'll need to restore it after the command
    // completes.
    vfs_node_t orig_dir = vfs->filesystem_interface->current_directory(vfs);
    
    // Construct an actual path from the string provided. If the path starts
    // at root, then navigate to root.
    vfs_path_node_t path_node = vfs_construct_path(path);
    if (path_node->is_root) {
        vfs->filesystem_interface->set_directory(vfs, NULL);
    }

    // Step through the path. Each time we can not find the appropriate
    // directory, create it.
    while (path_node) {
        const char *name = path_node->name;
        vfs_node_t dir = vfs->filesystem_interface->get_node(vfs, name);

        // Did we find the directory? If so was it a directory?
        if (dir->state == vfs_node_used &&
            (dir->attributes & vfs_node_directory_attribute) == 0)
        {
            vfs->filesystem_interface->set_directory(vfs, orig_dir);
            return 0;
        }

        // If there was no returned result then simply create the node.
        if (dir->state == vfs_node_unused || dir->state == vfs_node_available) {
            dir = vfs->filesystem_interface->create_dir(vfs, name, 0);
        }

        // Navigate into the directory and repeat the process on the next
        // component.
        vfs->filesystem_interface->set_directory(vfs, dir);
        
        // Go to the next path component.
        path_node = path_node->next;
    }

    vfs->filesystem_interface->set_directory(vfs, orig_dir);
    return 1;
}

void vfs_write(vfs_t vfs, const char *name, uint8_t *bytes, uint32_t size)
{
    assert(vfs);
    vfs_touch(vfs, name);
    vfs->filesystem_interface->write(vfs, name, bytes, size);
}

uint32_t vfs_read(vfs_t vfs, const char *name, uint8_t **bytes)
{
    assert(vfs);
    return vfs->filesystem_interface->read(vfs, name, (void **)bytes);
}

void vfs_remove(vfs_t vfs, const char *name)
{
    assert(vfs);
    vfs->filesystem_interface->remove(vfs, name);
}

vfs_node_t vfs_get_file(vfs_t vfs, const char *path)
{
    assert(vfs);

    // Get the current directory. We'll need to restore it after the command
    // completes.
    vfs_node_t orig_dir = vfs->filesystem_interface->current_directory(vfs);

    // Construct an actual path from the string provided. If the path starts
    // at root, then navigate to root.
    vfs_path_node_t path_node = vfs_construct_path(path);
    if (path_node->is_root) {
        vfs->filesystem_interface->set_directory(vfs, NULL);
    }

    // Step through the path. Each time we can not find the appropriate
    // directory, create it.
    while (path_node) {
        const char *name = path_node->name;
        vfs_node_t node = vfs->filesystem_interface->get_node(vfs, name);

        // Do we have a node? If not abort.
        if (!node) {
            vfs->filesystem_interface->set_directory(vfs, orig_dir);
            return NULL;
        }

        // Is this the last path_node? Is the directory attribute set?
        if (!path_node->next) {
            if (node->attributes & vfs_node_directory_attribute) {
                // Can't return a dictory here.
                vfs->filesystem_interface->set_directory(vfs, orig_dir);
                return NULL;
            }
            else {
                // We have our candidate!
                return node;
            }
        }

        // We need to navigate into the node. Ensure it's a directory, otherwise
        // fail.
        if ((node->attributes & vfs_node_directory_attribute) == 0) {
            vfs->filesystem_interface->set_directory(vfs, orig_dir);
            return NULL;
        }

        // Navigate into the directory and repeat the process on the next
        // component.
        vfs->filesystem_interface->set_directory(vfs, node);

        // Go to the next path component.
        path_node = path_node->next;
    }

    // If we get here, everything wen't wrong!
    return NULL;
}

uint32_t vfs_sector_count_of(vfs_t vfs, const char *path)
{
    assert(vfs);

    // Get the file. If there is no file return UINT32_MAX to denote no file.
    vfs_node_t file = vfs_get_file(vfs, path);
    if (!file) {
        return 0;
    }

    return file->sector_count;
}

uint32_t vfs_nth_sector_of(vfs_t vfs, uint32_t n, const char *path)
{
    assert(vfs);

    // Get the file. If there is no file return UINT32_MAX to denote no file.
    vfs_node_t file = vfs_get_file(vfs, path);
    if (!file) {
        return UINT32_MAX;
    }

    // Make sure that we have requested a valid sector number.
    if (n >= file->sector_count) {
        return UINT32_MAX;
    }

    return file->sectors[n];
}
