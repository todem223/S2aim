package com.fsociety.s2aim;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.graphics.Color;
import android.view.Gravity;

public class MainActivity extends Activity {
    static { System.loadLibrary("s2aim"); }

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setBackgroundColor(Color.BLACK);

        TextView t = new TextView(this);
        t.setText("s2aim — " + nativeVersion());
        t.setTextColor(Color.RED);
        t.setTextSize(20);

        root.addView(t);
        setContentView(root);

        nativeStart();
    }

    public native String nativeVersion();
    public native void nativeStart();
}
