/*
 * Copyright (C) 2016 Simon Fels <morphis@gravedo.de>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

package org.freedesktop.autolauncher;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.content.Intent;

import java.util.*;
import java.util.regex.*;
import java.io.*;

public final class LauncherActivity extends Activity {
    private static final String TAG = "**** autolauncher";

    @Override
    public void onCreate(Bundle info) {
        Log.i(TAG, "onCreate");
        super.onCreate(info);
    }

    private String getProperty(String name, String defaultValue) {
      ArrayList<String> processList = new ArrayList<String>();
      String line;
      Pattern pattern = Pattern.compile("\\[(.+)\\]: \\[(.+)\\]");
      Matcher m;

      try {
          Process p = Runtime.getRuntime().exec("getprop");
          BufferedReader input = new BufferedReader(new InputStreamReader(p.getInputStream()));
          while ((line = input.readLine()) != null) {
              processList.add(line);
              m = pattern.matcher(line);
              if (m.find()) {
                  MatchResult result = m.toMatchResult();
                  if(result.group(1).equals(name))
                      return result.group(2);
              }
          }
          input.close();
      } catch (Exception err) {
          err.printStackTrace();
      }

      return defaultValue;
    }

    @Override
    public void onStart() {
        Log.i(TAG, "onStart");
        super.onStart();
        
        Intent intent = getPackageManager().getLaunchIntentForPackage(getProperty("spurv.application", ""));
        startActivity(intent);
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy");
        super.onDestroy();
    }
}
