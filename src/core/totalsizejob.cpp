#include "totalsizejob.h"

namespace Fm2 {

static const char query_str[] =
    G_FILE_ATTRIBUTE_STANDARD_TYPE","
    G_FILE_ATTRIBUTE_STANDARD_NAME","
    G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL","
    G_FILE_ATTRIBUTE_STANDARD_SIZE","
    G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE","
    G_FILE_ATTRIBUTE_ID_FILESYSTEM;

TotalSizeJob::TotalSizeJob() {

}

void TotalSizeJob::run(FilePath& path, GObjectPtr<GFileInfo> &inf) {
    GError* err = NULL;
    GFileType type;
    const char* fs_id;
    bool descend;

_retry_query_info:
    if(!inf) {
        inf = GObjectPtr<GFileInfo> {
            g_file_query_info(path.gfile().get(), query_str,
            (flags & FOLLOW_LINKS) ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
            cancellable().get(), &err),
            false
        };
        if(!inf) {
#if 0
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_error_free(err);
            err = NULL;
            if(act == FM_JOB_RETRY) {
                goto _retry_query_info;
            }
            return;
#endif
        }
    }
    if(isCancelled()) {
        return;
    }

    type = g_file_info_get_file_type(inf.get());
    descend = true;

    ++count;
    /* SF bug #892: dir file size is not relevant in the summary */
    if(type != G_FILE_TYPE_DIRECTORY) {
        total_size += g_file_info_get_size(inf.get());
    }
    total_ondisk_size += g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE);

    /* prepare for moving across different devices */
    if(flags & PREPARE_MOVE) {
        fs_id = g_file_info_get_attribute_string(inf.get(), G_FILE_ATTRIBUTE_ID_FILESYSTEM);
        fs_id = g_intern_string(fs_id);
        if(g_strcmp0(fs_id, dest_fs_id) != 0) {
            /* files on different device requires an additional 'delete' for the source file. */
            ++total_size; /* this is for the additional delete */
            ++total_ondisk_size;
            ++count;
        }
        else {
            descend = false;
        }
    }

    if(type == G_FILE_TYPE_DIRECTORY) {
#if 0
        FmPath* fm_path = fm_path_new_for_gfile(gf);
        /* check if we need to decends into the dir. */
        /* trash:/// doesn't support deleting files recursively */
        if(flags & PREPARE_DELETE && fm_path_is_trash(fm_path) && ! fm_path_is_trash_root(fm_path)) {
            descend = false;
        }
        else {
            /* only descends into files on the same filesystem */
            if(flags & FM_DC_JOB_SAME_FS) {
                fs_id = g_file_info_get_attribute_string(inf, G_FILE_ATTRIBUTE_ID_FILESYSTEM);
                descend = (g_strcmp0(fs_id, dest_fs_id) == 0);
            }
        }
        fm_path_unref(fm_path);
#endif
        inf = nullptr;

        if(descend) {
_retry_enum_children:
            auto enu = GObjectPtr<GFileEnumerator> {
                g_file_enumerate_children(path.gfile().get(), query_str,
                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                cancellable().get(), &err),
                false
            };
            if(enu) {
                while(!isCancelled()) {
                    inf = g_file_enumerator_next_file(enu.get(), cancellable().get(), &err);
                    if(inf) {
                        FilePath child = path.child(g_file_info_get_name(inf.get()));
                        run(child, inf);
                    }
                    else {
                        if(err) { /* error! */
#if 0
                            /* FM_JOB_RETRY is not supported */
                            /*FmJobErrorAction act = */
                            fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
                            g_error_free(err);
                            err = NULL;
#endif
                        }
                        else {
                            /* EOF is reached, do nothing. */
                            break;
                        }
                    }
                }
                g_file_enumerator_close(enu.get(), NULL, NULL);
            }
            else {
#if 0
                FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
                g_error_free(err);
                err = NULL;
                if(act == FM_JOB_RETRY) {
                    goto _retry_enum_children;
                }
#endif
            }
        }
    }
}


void TotalSizeJob::run() {
    for(auto& path : paths_) {
        GObjectPtr<GFileInfo> inf;
        run(path, inf);
    }
    Q_EMIT finished();
}


} // namespace Fm2
