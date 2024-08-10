#include "filesysteminfojob.h"
#include "gobjectptr.h"

namespace Fm {

void FileSystemInfoJob::exec() {
    GObjectPtr<GFileInfo> inf = GObjectPtr<GFileInfo>{
            g_file_query_filesystem_info(
                path_.gfile().get(),
                G_FILE_ATTRIBUTE_FILESYSTEM_SIZE","
                G_FILE_ATTRIBUTE_FILESYSTEM_FREE","
                G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE,
                cancellable().get(), nullptr),
            false
    };
    if(!inf)
        return;
    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_SIZE)) {
        size_ = g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
        freeSize_ = g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
        isAvailable_ = true;
    }
    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE)) {
        isRemote_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE);
    }
}

} // namespace Fm
