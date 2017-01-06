#include "dirlistjob.h"
#include <gio/gio.h>
#include "fileinfo_p.h"
#include "gioptrs.h"
#include <QDebug>

namespace Fm2 {

DirListJob::DirListJob(const FilePath& path, Flags flags): dir_path{path} {
}

void DirListJob::run() {
_retry:
    GErrorPtr err;
    GFileInfoPtr dir_inf{
        g_file_query_info(dir_path.gfile().get(), gfile_info_query_attribs,
                          G_FILE_QUERY_INFO_NONE, cancellable().get(), &err),
        false
    };
    if(!dir_inf) {
        ErrorAction act = emitError(err, ErrorSeverity::MODERATE);
        if(act == ErrorAction::RETRY) {
            err.reset();
            goto _retry;
        }
        return;
    }

    if(g_file_info_get_file_type(dir_inf.get()) != G_FILE_TYPE_DIRECTORY) {
        auto path_str = dir_path.toString();
        err = GErrorPtr{
                G_IO_ERROR,
                G_IO_ERROR_NOT_DIRECTORY,
                tr("The specified directory '%1' is not valid").arg(path_str.get())
        };
        emitError(err, ErrorSeverity::CRITICAL);
        return;
    }
    else {
        std::lock_guard<std::mutex> lock{mutex_};
        dir_fi = std::make_shared<FileInfo>(dir_inf);
    }

    FileInfoList foundFiles;
    /* check if FS is R/O and set attr. into inf */
    // FIXME:  _fm_file_info_job_update_fs_readonly(gf, inf, NULL, NULL);
    GFileEnumeratorPtr enu = GFileEnumeratorPtr{
            g_file_enumerate_children(dir_path.gfile().get(), gfile_info_query_attribs,
                                      G_FILE_QUERY_INFO_NONE, cancellable().get(), &err),
            false
    };
    if(enu) {
        while(!isCancelled()) {
            err.reset();
            GFileInfoPtr inf{g_file_enumerator_next_file(enu.get(), cancellable().get(), &err), false};
            if(inf) {
#if 0
                FmPath* dir, *sub;
                GFile* child;
                if(G_UNLIKELY(job->flags & FM_DIR_LIST_JOB_DIR_ONLY)) {
                    /* FIXME: handle symlinks */
                    if(g_file_info_get_file_type(inf) != G_FILE_TYPE_DIRECTORY) {
                        g_object_unref(inf);
                        continue;
                    }
                }

                /* virtual folders may return children not within them */
                dir = fm_path_new_for_gfile(g_file_enumerator_get_container(enu));
                if(fm_path_equal(job->dir_path, dir)) {
                    sub = fm_path_new_child(job->dir_path, g_file_info_get_name(inf));
                }
                else {
                    sub = fm_path_new_child(dir, g_file_info_get_name(inf));
                }
                child = g_file_get_child(g_file_enumerator_get_container(enu),
                                         g_file_info_get_name(inf));
                if(g_file_info_get_file_type(inf) == G_FILE_TYPE_DIRECTORY)
                    /* for dir: check if its FS is R/O and set attr. into inf */
                {
                    _fm_file_info_job_update_fs_readonly(child, inf, NULL, NULL);
                }
                fi = fm_file_info_new_from_g_file_data(child, inf, sub);
                fm_path_unref(sub);
                fm_path_unref(dir);
                g_object_unref(child);
#endif
                auto fileInfo = std::make_shared<FileInfo>(inf);
                if(emit_files_found) {
                    // Q_EMIT filesFound();
                }
                foundFiles.push_back(std::move(fileInfo));
            }
            else {
                if(err) {
                    ErrorAction act = emitError(err, ErrorSeverity::MILD);
                    /* ErrorAction::RETRY is not supported. */
                    if(act == ErrorAction::ABORT) {
                        cancel();
                    }
                }
                /* otherwise it's EOL */
                break;
            }
        }
        g_file_enumerator_close(enu.get(), cancellable().get(), &err);
    }
    else {
        emitError(err, ErrorSeverity::CRITICAL);
    }

    if(!foundFiles.empty()) {
        std::lock_guard<std::mutex> lock{mutex_};
        files_.swap(foundFiles);
    }

    Q_EMIT finished();
}

#if 0
//FIXME: incremental..

static gboolean emit_found_files(gpointer user_data) {
    /* this callback is called from the main thread */
    FmDirListJob* job = FM_DIR_LIST_JOB(user_data);
    /* g_print("emit_found_files: %d\n", g_slist_length(job->files_to_add)); */

    if(g_source_is_destroyed(g_main_current_source())) {
        return FALSE;
    }
    g_signal_emit(job, signals[FILES_FOUND], 0, job->files_to_add);
    g_slist_free_full(job->files_to_add, (GDestroyNotify)fm_file_info_unref);
    job->files_to_add = NULL;
    job->delay_add_files_handler = 0;
    return FALSE;
}

static gpointer queue_add_file(FmJob* fmjob, gpointer user_data) {
    FmDirListJob* job = FM_DIR_LIST_JOB(fmjob);
    FmFileInfo* file = FM_FILE_INFO(user_data);
    /* this callback is called from the main thread */
    /* g_print("queue_add_file: %s\n", fm_file_info_get_disp_name(file)); */
    job->files_to_add = g_slist_prepend(job->files_to_add, fm_file_info_ref(file));
    if(job->delay_add_files_handler == 0)
        job->delay_add_files_handler = g_timeout_add_seconds_full(G_PRIORITY_LOW,
                                       1, emit_found_files, g_object_ref(job), g_object_unref);
    return NULL;
}

void fm_dir_list_job_add_found_file(FmDirListJob* job, FmFileInfo* file) {
    fm_file_info_list_push_tail(job->files, file);
    if(G_UNLIKELY(job->emit_files_found)) {
        fm_job_call_main_thread(FM_JOB(job), queue_add_file, file);
    }
}
#endif

} // namespace Fm2
