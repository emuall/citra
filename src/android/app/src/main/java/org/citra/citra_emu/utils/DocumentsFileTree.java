package org.citra.citra_emu.utils;

import android.content.Context;
import android.net.Uri;
import android.os.ParcelFileDescriptor;

import org.citra.citra_emu.CitraApplication;

import java.io.File;

/**
 * A cached document tree for citra user directory.
 * For every filepath which is not startsWith "content://" will need to use this class to traverse.
 * For example:
 * C++ citra log file directory will be /log/citra_log.txt.
 * After DocumentsTree.resolvePath() it will become content URI.
 */
public class DocumentsFileTree {
    private String uri;
    private boolean isDirectory = true;
    private final Context context;
    public static final String DELIMITER = "/";

    public DocumentsFileTree() {
        context = CitraApplication.getAppContext();
    }

    public void setRoot(Uri rootUri) {
        uri = rootUri.toString();
        isDirectory = true;
    }

    public boolean createFile(String filepath, String name) {
        Log.error("createFile : " + filepath);
        try {
            File pathFile = new File(resolvePath(filepath));
            pathFile.mkdirs();
            return new File(pathFile, name).createNewFile();
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean createDir(String filepath, String name) {
        Log.error("createDir : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        return new File(pathFile, name).mkdirs();
    }

    public int openContentUri(String filepath, String openmode) {
        try {
            Log.error("openContentUri : " + filepath);
            ParcelFileDescriptor parcelFileDescriptor =
                    ParcelFileDescriptor.open(new File(resolvePath(filepath)), ParcelFileDescriptor.parseMode(openmode));
            return parcelFileDescriptor.detachFd();
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
    }

    public String getFilename(String filepath) {
        Log.error("getFilename : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        return pathFile.getName();
    }

    public String[] getFilesName(String filepath) {
        Log.error("getFilesName : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        if (pathFile.exists()) {
            return pathFile.list();
        } else {
            return new String[0];
        }
    }

    public long getFileSize(String filepath) {
        Log.error("getFileSize : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        return pathFile.length();
    }

    public boolean isDirectory(String filepath) {
        File pathFile = new File(resolvePath(filepath));
        return pathFile.isDirectory();
    }

    public boolean Exists(String filepath) {
        Log.error("Exists : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        return pathFile.exists();
    }

    public boolean copyFile(String sourcePath, String destinationParentPath, String destinationFilename) {
        Log.error("copyFile : " + sourcePath);
//        throw new UnsupportedOperationException("");
        return false;
    }

    public boolean renameFile(String filepath, String destinationFilename) {
        Log.error("renameFile : " + filepath);
        Log.error("deleteDocument : " + filepath);
        return new File(resolvePath(filepath)).renameTo(new File(resolvePath(destinationFilename)));
    }

    public boolean deleteDocument(String filepath) {
        Log.error("deleteDocument : " + filepath);
        String path = resolvePath(filepath);
        return new File(path).delete();
    }

    private String resolvePath(String filepath) {
        Log.error("resolvePath : " + filepath);
        return new File(uri, filepath).getAbsolutePath();
    }

}
