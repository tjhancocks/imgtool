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

#ifndef VFS_INTERFACE
#define VFS_INTERFACE

#include <device/virtual.h>
#include <vfs/node.h>

struct vfs;
struct vfs_directory;

struct vfs_interface {
    const char *(*type_name)();
    void (*format_device)(vdevice_t dev, const char *name, uint8_t *bootcode);
    void *(*mount_filesystem)(struct vfs *fs);
    void (*unmount_filesystem)(struct vfs *fs);
    void (*set_directory)(struct vfs *fs, struct vfs_directory *dir);
    void *(*list_directory)(struct vfs *fs);
    void (*create_file)(struct vfs *fs,
                        const char *filename,
                        enum vfs_node_attributes attributes);
    void (*create_dir)(struct vfs *fs,
                       const char *name,
                       enum vfs_node_attributes a);
    void (*write)(struct vfs *fs, const char *name, void *bytes, uint32_t n);
};

typedef struct vfs_interface * vfs_interface_t;

vfs_interface_t vfs_interface_for_device(vdevice_t dev);
vfs_interface_t vfs_interface_for(const char *type);

vfs_interface_t vfs_interface_init();
void vfs_interface_destroy(vfs_interface_t interface);

#endif
