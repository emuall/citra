// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package org.citra.citra_emu;

import android.app.Application;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Build;

import org.citra.citra_emu.model.GameDatabase;
import org.citra.citra_emu.utils.DirectoryInitialization;
import org.citra.citra_emu.utils.DocumentsFileTree;
import org.citra.citra_emu.utils.DocumentsTree;
import org.citra.citra_emu.utils.PermissionsHandler;

public class CitraApplication extends Application {
    public static GameDatabase databaseHelper;
    public static DocumentsFileTree documentsTree;
    private static Application application;

    private static void createNotificationChannel(Application app) {
        // Create the NotificationChannel, but only on API 26+ because
        // the NotificationChannel class is new and not in the support library
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            return;
        }
        NotificationManager notificationManager = app.getSystemService(NotificationManager.class);
        {
            // General notification
            CharSequence name = app.getString(R.string.app_notification_channel_name);
            String description = app.getString(R.string.app_notification_channel_description);
            NotificationChannel channel = new NotificationChannel(
                    app.getString(R.string.app_notification_channel_id), name,
                    NotificationManager.IMPORTANCE_LOW);
            channel.setDescription(description);
            channel.setSound(null, null);
            channel.setVibrationPattern(null);

            notificationManager.createNotificationChannel(channel);
        }
        {
            // CIA Install notifications
            NotificationChannel channel = new NotificationChannel(
                    app.getString(R.string.cia_install_notification_channel_id),
                    app.getString(R.string.cia_install_notification_channel_name),
                    NotificationManager.IMPORTANCE_DEFAULT);
            channel.setDescription(app.getString(R.string.cia_install_notification_channel_description));
            channel.setSound(null, null);
            channel.setVibrationPattern(null);

            notificationManager.createNotificationChannel(channel);
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        init(this);
    }

    public static void init(Application app) {

        application = app;
        documentsTree = new DocumentsFileTree();

//        if (PermissionsHandler.hasWriteAccess(getApplicationContext())) {
        DirectoryInitialization.start(app);
//        }

        NativeLibrary.LogDeviceInfo();
        createNotificationChannel(app);

        databaseHelper = new GameDatabase(app);
    }

    public static void setAppContext(Application application) {
        init(application);
    }

    public static Context getAppContext() {
        if (application == null) {
            return null;
        }
        return application;
    }
}
