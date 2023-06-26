package org.citra.citra_emu.utils;

import android.content.Context;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.text.TextUtils;

import org.citra.citra_emu.CitraApplication;

import java.io.File;

/**
 * A cached document tree for citra user directory.
 * For every filepath which is not startsWith "content://" will need to use this class to traverse.
 * For example:
 * C++ citra log file directory will be /log/citra_log.txt.
 * After DocumentsTree.resolvePath() it will become content URI.
 */
public class DocumentsFileTree extends DocumentsTree {
    private static String TAG = DocumentsFileTree.class.getSimpleName();
    private static String uri;
    private boolean isDirectory = true;

    public DocumentsFileTree() {
    }

    public void setRoot(Uri rootUri) {
        uri = rootUri.toString();
        isDirectory = true;
        Log.error(TAG,"processName: " + ProcessUtil.getCurrentProcessName(CitraApplication.getAppContext()));
        Log.error(TAG, "setRoot : " + "uri:" + uri);
    }

    public boolean createFile(String filepath, String name) {
        Log.error(TAG, "createFile : " + filepath);
        try {
            File pathFile = new File(resolvePath(filepath));
            pathFile.mkdirs();
            return new File(pathFile, name).createNewFile();
        } catch (Exception e) {
            Log.error(TAG, "createFile error : " + filepath);
            e.printStackTrace();
            return false;
        }
    }

    public boolean createDir(String filepath, String name) {
        Log.error(TAG, "createDir : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        return new File(pathFile, name).mkdirs();
    }

    public int openContentUri(String filepath, String openmode) {
        try {
            Log.error(TAG, "openContentUri : " + filepath);
            ParcelFileDescriptor parcelFileDescriptor =
                    ParcelFileDescriptor.open(new File(resolvePath(filepath)), ParcelFileDescriptor.parseMode(openmode));
            return parcelFileDescriptor.detachFd();
        } catch (Exception e) {
            Log.error(TAG, "openContentUri error: " + filepath);
            e.printStackTrace();
            return -1;
        }
    }

    public String getFilename(String filepath) {
        Log.error(TAG, "getFilename : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        return pathFile.getName();
    }

    public String[] getFilesName(String filepath) {
        Log.error(TAG, "getFilesName : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        if (pathFile.exists()) {
            return pathFile.list();
        } else {
            return new String[0];
        }
    }

    public long getFileSize(String filepath) {
        Log.error(TAG, "getFileSize : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        return pathFile.length();
    }

    public boolean isDirectory(String filepath) {
        Log.error(TAG, "isDirectory : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        Log.error(TAG, "---isDirectory : " + pathFile);
        Log.error(TAG, "---isDirectory : " + pathFile.isDirectory());
        return pathFile.isDirectory();
    }

    public boolean Exists(String filepath) {
        Log.error(TAG, "Exists : " + filepath);
        File pathFile = new File(resolvePath(filepath));
        return pathFile.exists();
    }

    public boolean copyFile(String sourcePath, String destinationParentPath, String destinationFilename) {
        Log.error(TAG, "copyFile : " + sourcePath + "d:" + destinationParentPath + " df:" + destinationFilename);
        throw new UnsupportedOperationException("");
    }

    public boolean renameFile(String filepath, String destinationFilename) {
        Log.error(TAG, "renameFile : " + filepath);
        return new File(resolvePath(filepath)).renameTo(new File(resolvePath(destinationFilename)));
    }

    public boolean deleteDocument(String filepath) {
        Log.error(TAG, "deleteDocument : " + filepath);
        String path = resolvePath(filepath);
        return new File(path).delete();
    }

    private String resolvePath(String filepath) {
        Log.error(TAG, "--uri : " + uri);
        Log.error(TAG, "resolvePath : " + filepath);
        if (filepath.contains(CitraApplication.getAppContext().getPackageName())) {
            return filepath;
        } else {
            return new File(uri, filepath).getAbsolutePath();
        }
    }

}
