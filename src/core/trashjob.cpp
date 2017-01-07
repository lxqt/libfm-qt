#include "trashjob.h"

namespace Fm2 {

TrashJob::TrashJob(const FilePathList& paths): paths_{paths} {
}

TrashJob::TrashJob(const FilePathList&& paths): paths_{paths} {
}

void TrashJob::run() {
    setTotalAmount(paths_.size(), paths_.size());
    Q_EMIT preparedToRun();

    /* FIXME: we shouldn't trash a file already in trash:/// */
    for(auto& path : paths_) {
        if(isCancelled()) {
            break;
        }

        setCurrentFile(path);

        for(;;) {
            GErrorPtr err;
            GFile* gf = path.gfile().get();
            GFileInfoPtr inf{
                g_file_query_info(gf, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, G_FILE_QUERY_INFO_NONE,
                cancellable().get(), &err),
                false
            };

            bool ret = FALSE;
            if(fm_config->no_usb_trash) {
                err.reset();
                GMountPtr mnt{g_file_find_enclosing_mount(gf, NULL, &err), false};
                if(mnt) {
                    ret = g_mount_can_unmount(mnt.get()); /* TRUE if it's removable media */
                    if(ret) {
                        unsupportedFiles_.push_back(path);
                    }
                }
            }

            if(!ret) {
                err.reset();
                ret = g_file_trash(gf, cancellable().get(), &err);
            }
            if(!ret) {
                /* if trashing is not supported by the file system */
                if(err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_NOT_SUPPORTED) {
                    unsupportedFiles_.push_back(path);
                }
                else {
                    ErrorAction act = emitError(err, ErrorSeverity::MODERATE);
                    if(act == ErrorAction::RETRY) {
                        err.reset();
                    }
                    else if(act == ErrorAction::ABORT) {
                        cancel();
                        return;
                    }
                    else {
                        break;
                    }
                }
            }
        }
        addFinishedAmount(1, 1);
    }
}


} // namespace Fm2
