#include "dirlistjob.h"
#include <gio/gio.h>
#include "fileinfo_p.h"
#include "gioptrs.h"
#include <QDebug>

namespace Fm {

DirListJob::DirListJob(const FilePath& path, Flags _flags):
    dir_path{path}, flags{_flags}, emit_files_found{false} {
}

void DirListJob::setIncremental(bool set) {
    emit_files_found = set;
}

void DirListJob::exec() {
    GErrorPtr err;
    GFileInfoPtr dir_inf;
    GFilePtr dir_gfile = dir_path.gfile();
    // FIXME: these are hacks for search:/// URI implemented by libfm which contains some bugs
    bool isFileSearch = dir_path.hasUriScheme("search");
    if(isFileSearch) {
        // NOTE: The GFile instance changes its URI during file enumeration (bad design).
        // So we create a copy here to avoid channging the gfile stored in dir_path.
        // FIXME: later we should refactor file search and remove this dirty hack.
        dir_gfile = GFilePtr{g_file_dup(dir_gfile.get())};
    }
_retry:
    err.reset();
    dir_inf = GFileInfoPtr{
        g_file_query_info(dir_gfile.get(), defaultGFileInfoQueryAttribs,
                          G_FILE_QUERY_INFO_NONE, cancellable().get(), &err),
        false
    };
    if(!dir_inf) {
        ErrorAction act = emitError(err, err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_CANCELLED
                                         ? ErrorSeverity::MILD // may happen with MTP
                                         : ErrorSeverity::MODERATE);
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
                tr("The specified directory '%1' is not valid").arg(QString::fromUtf8(path_str.get()))
        };
        emitError(err, ErrorSeverity::CRITICAL);
        return;
    }
    else {
        // First set the attributes "filesystem::readonly" and "filesystem::remote",
        // which will be queried by FileInfo and are useful only for the parent dir.
        if(GFileInfoPtr fs_info{
            g_file_query_filesystem_info(dir_gfile.get(),
                                         G_FILE_ATTRIBUTE_FILESYSTEM_READONLY","
                                         G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE,
                                         cancellable().get(), nullptr),
            false
        }) {
            if(g_file_info_has_attribute(fs_info.get(), G_FILE_ATTRIBUTE_FILESYSTEM_READONLY)) {
                g_file_info_set_attribute_boolean(dir_inf.get(),
                                                  G_FILE_ATTRIBUTE_FILESYSTEM_READONLY,
                                                  g_file_info_get_attribute_boolean(fs_info.get(),
                                                                                    G_FILE_ATTRIBUTE_FILESYSTEM_READONLY));
            }
            if(g_file_info_has_attribute(fs_info.get(), G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE)) {
                g_file_info_set_attribute_boolean(dir_inf.get(),
                                                  G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE,
                                                  g_file_info_get_attribute_boolean(fs_info.get(),
                                                                                    G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE));
            }
        }

        std::lock_guard<std::mutex> lock{mutex_};
        dir_fi = std::make_shared<FileInfo>(dir_inf, dir_path);
    }

    FileInfoList foundFiles;
    FileInfoList incrementalBatch;
    // batch size trades round-trip overhead against result latency;
    // revisit if very large recursive incremental listing feels laggy.
    constexpr std::size_t incrementalBatchSize = 32;
    /* check if FS is R/O and set attr. into inf */
    // FIXME:  _fm_file_info_job_update_fs_readonly(gf, inf, nullptr, nullptr);
    err.reset();
    GFileEnumeratorPtr enu = GFileEnumeratorPtr{
            g_file_enumerate_children(dir_gfile.get(), defaultGFileInfoQueryAttribs,
                                      G_FILE_QUERY_INFO_NONE, cancellable().get(), &err),
            false
    };
    if(enu) {
        // qDebug() << "START LISTING:" << dir_path.toString().get();
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
#endif
                // virtual folders may return children not within them
                // For example: the search:/// URI implemented by libfm might return files from different folders during enumeration.
                // So here we call g_file_enumerator_get_container() to get the real parent path rather than simply using dir_path.
                // This is not the behaviour of gio, but the extensions by libfm might do this.
                // FIXME: after we port these vfs implementation from libfm, we can redesign this.
                FilePath realParentPath = FilePath{g_file_enumerator_get_container(enu.get()), true};
                if(isFileSearch) { // this is a file sarch job (search:/// URI)
                    // FIXME: redesign file search and remove this dirty hack
                    // the libfm implementation of search:/// URI returns a customized GFile implementation that does not behave normally.
                    // let's get its actual URI and re-create a normal gio GFile instance from it.
                    realParentPath = FilePath::fromUri(realParentPath.uri().get());
                }
#if 0
                if(g_file_info_get_file_type(inf) == G_FILE_TYPE_DIRECTORY)
                    /* for dir: check if its FS is R/O and set attr. into inf */
                {
                    _fm_file_info_job_update_fs_readonly(child, inf, nullptr, nullptr);
                }
                fi = fm_file_info_new_from_g_file_data(child, inf, sub);
#endif
                auto fileInfo = std::make_shared<FileInfo>(inf, FilePath(), realParentPath);
                if(emit_files_found) {
                    incrementalBatch.push_back(fileInfo);
                    if(incrementalBatch.size() >= incrementalBatchSize) {
                        Q_EMIT filesFound(incrementalBatch);
                        incrementalBatch.clear();
                    }
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
        if(emit_files_found && !incrementalBatch.empty()) {
            Q_EMIT filesFound(incrementalBatch);
            incrementalBatch.clear();
        }
        err.reset();
        g_file_enumerator_close(enu.get(), cancellable().get(), &err);
    }
    else {
        emitError(err, err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_CANCELLED
                       ? ErrorSeverity::MILD // may happen at Folder::reload()
                       : ErrorSeverity::CRITICAL);
    }

    // qDebug() << "END LISTING:" << dir_path.toString().get();
    if(!foundFiles.empty()) {
        std::lock_guard<std::mutex> lock{mutex_};
        files_.swap(foundFiles);
    }
}

} // namespace Fm
