package com.night.backgroundchange;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;

public class MainActivity extends Activity {

    // Load the native C++ library
    static {
        System.loadLibrary("native-lib");
    }

    // Declare native C++ functions
    public native String getRandomColorHex();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final RelativeLayout mainLayout = findViewById(R.id.main_layout);
        Button changeColorButton = findViewById(R.id.btn_change_color);

        changeColorButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Fetch random color hex string from C++ backend layer
                String colorHex = getRandomColorHex();
                mainLayout.setBackgroundColor(Color.parseColor(colorHex));
            }
        });
    }
}
