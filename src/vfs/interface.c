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

#include <string.h>
#include <stdlib.h>
#include <vfs/interface.h>

#include <fat/fat12.h>


vfs_interface_t vfs_interface_init()
{
    return calloc(1, sizeof(struct vfs_interface));
}

void vfs_interface_destroy(vfs_interface_t interface)
{
    free(interface);
}

vfs_interface_t vfs_interface_for(const char *type)
{
    if (strcmp(type, "fat12") == 0) {
        return fat12_init();
    }
    return NULL;
}

vfs_interface_t vfs_interface_for_device(vdevice_t dev)
{
    if (fat12_test(dev, NULL)) {
        return fat12_init();
    }
    return NULL;
}
