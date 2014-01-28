/* -*- Mode: Java; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.widget;

import org.mozilla.gecko.animation.ViewHelper;

import android.content.Context;
import android.graphics.Rect;
import android.os.Build;
import android.view.MotionEvent;
import android.widget.ViewFlipper;
import android.util.AttributeSet;

/* This extends the normal ViewFlipper only to fix bug 956075 on < 3.0 devices.
 * i.e. It ignores touch events on the ViewFlipper when its hidden. */

public class GeckoViewFlipper extends ViewFlipper {
    private static final String LOGTAG = "GeckoViewFlipper";
    private Rect mRect = new Rect();

    public GeckoViewFlipper(Context context) {
        super(context);
    }

    public GeckoViewFlipper(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (Build.VERSION.SDK_INT < 11) {
            // Fix bug 956075. Don't allow touching this View if its hidden.
            getHitRect(mRect);
            mRect.offset((int) ViewHelper.getTranslationX(this), (int) ViewHelper.getTranslationY(this));

            if (!mRect.contains((int) ev.getRawX(), (int) ev.getRawY())) {
                return false;
            }
        }

        return super.dispatchTouchEvent(ev);
    }
}
