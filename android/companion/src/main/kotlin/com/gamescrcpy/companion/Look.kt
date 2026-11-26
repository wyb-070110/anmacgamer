package com.gamescrcpy.companion

import android.animation.ValueAnimator
import android.content.Context
import android.content.res.Configuration
import android.graphics.Typeface
import android.os.Build
import android.util.TypedValue
import android.view.animation.PathInterpolator
import android.widget.TextView
import kotlin.math.roundToInt

// The colour roles of Material 3, as Android 17 uses them: from the wallpaper (dynamic colour) where the phone has
// it, otherwise a palette made from the green of the app icon.
data class Palette(
    val dark: Boolean,
    val primary: Int,
    val onPrimary: Int,
    val primaryContainer: Int,
    val onPrimaryContainer: Int,
    val secondaryContainer: Int,
    val onSecondaryContainer: Int,
    val tertiaryContainer: Int,
    val onTertiaryContainer: Int,
    val surface: Int,
    val surfaceLowest: Int,
    val surfaceLow: Int,
    val surfaceContainer: Int,
    val surfaceHigh: Int,
    val surfaceHighest: Int,
    val onSurface: Int,
    val onSurfaceVariant: Int,
    val outline: Int,
    val outlineVariant: Int,
    val error: Int,
    val errorContainer: Int,
    val onErrorContainer: Int,
) {
    // The page behind the grouped lists, and the lists themselves (Settings on Android 17).
    val page: Int
        get() = if (dark) surface else surfaceContainer

    val item: Int
        get() = if (dark) surfaceLow else surfaceLowest

    val press: Int
        get() = Look.alpha(onSurface, PRESS)

    val success: Int
        get() = if (dark) 0xFF7DD99F.toInt() else 0xFF1E7A43.toInt()

    val warning: Int
        get() = if (dark) 0xFFF2C26B.toInt() else 0xFF8A5A00.toInt()

    companion object {
        private const val PRESS = 0.10f

        fun green(dark: Boolean): Palette = if (dark) {
            Palette(
                dark = true,
                primary = 0xFF6CDBAC.toInt(),
                onPrimary = 0xFF003825.toInt(),
                primaryContainer = 0xFF005138.toInt(),
                onPrimaryContainer = 0xFF89F8C7.toInt(),
                secondaryContainer = 0xFF334B3F.toInt(),
                onSecondaryContainer = 0xFFCEE9DA.toInt(),
                tertiaryContainer = 0xFF264D5A.toInt(),
                onTertiaryContainer = 0xFFC1E9FB.toInt(),
                surface = 0xFF0F1511.toInt(),
                surfaceLowest = 0xFF0A0F0C.toInt(),
                surfaceLow = 0xFF171D1A.toInt(),
                surfaceContainer = 0xFF1B211E.toInt(),
                surfaceHigh = 0xFF252B28.toInt(),
                surfaceHighest = 0xFF303633.toInt(),
                onSurface = 0xFFDEE4DE.toInt(),
                onSurfaceVariant = 0xFFBFC9C1.toInt(),
                outline = 0xFF8A938C.toInt(),
                outlineVariant = 0xFF404943.toInt(),
                error = 0xFFFFB4AB.toInt(),
                errorContainer = 0xFF93000A.toInt(),
                onErrorContainer = 0xFFFFDAD6.toInt(),
            )
        } else {
            Palette(
                dark = false,
                primary = 0xFF006C4C.toInt(),
                onPrimary = 0xFFFFFFFF.toInt(),
                primaryContainer = 0xFF89F8C7.toInt(),
                onPrimaryContainer = 0xFF002114.toInt(),
                secondaryContainer = 0xFFCEE9DA.toInt(),
                onSecondaryContainer = 0xFF092016.toInt(),
                tertiaryContainer = 0xFFC1E9FB.toInt(),
                onTertiaryContainer = 0xFF001F29.toInt(),
                surface = 0xFFF5FBF5.toInt(),
                surfaceLowest = 0xFFFFFFFF.toInt(),
                surfaceLow = 0xFFEFF5EF.toInt(),
                surfaceContainer = 0xFFE9EFE9.toInt(),
                surfaceHigh = 0xFFE4EAE4.toInt(),
                surfaceHighest = 0xFFDEE4DE.toInt(),
                onSurface = 0xFF171D1A.toInt(),
                onSurfaceVariant = 0xFF404943.toInt(),
                outline = 0xFF707973.toInt(),
                outlineVariant = 0xFFBFC9C1.toInt(),
                error = 0xFFBA1A1A.toInt(),
                errorContainer = 0xFFFFDAD6.toInt(),
                onErrorContainer = 0xFF410002.toInt(),
            )
        }

        // Android 14 and later name every role of the wallpaper's scheme.
        @Suppress("CyclomaticComplexMethod")
        fun dynamic(context: Context, dark: Boolean): Palette {
            fun c(light: Int, night: Int): Int = context.getColor(if (dark) night else light)
            return Palette(
                dark = dark,
                primary = c(android.R.color.system_primary_light, android.R.color.system_primary_dark),
                onPrimary = c(android.R.color.system_on_primary_light, android.R.color.system_on_primary_dark),
                primaryContainer = c(
                    android.R.color.system_primary_container_light,
                    android.R.color.system_primary_container_dark,
                ),
                onPrimaryContainer = c(
                    android.R.color.system_on_primary_container_light,
                    android.R.color.system_on_primary_container_dark,
                ),
                secondaryContainer = c(
                    android.R.color.system_secondary_container_light,
                    android.R.color.system_secondary_container_dark,
                ),
                onSecondaryContainer = c(
                    android.R.color.system_on_secondary_container_light,
                    android.R.color.system_on_secondary_container_dark,
                ),
                tertiaryContainer = c(
                    android.R.color.system_tertiary_container_light,
                    android.R.color.system_tertiary_container_dark,
                ),
                onTertiaryContainer = c(
                    android.R.color.system_on_tertiary_container_light,
                    android.R.color.system_on_tertiary_container_dark,
                ),
                surface = c(android.R.color.system_surface_light, android.R.color.system_surface_dark),
                surfaceLowest = c(
                    android.R.color.system_surface_container_lowest_light,
                    android.R.color.system_surface_container_lowest_dark,
                ),
                surfaceLow = c(
                    android.R.color.system_surface_container_low_light,
                    android.R.color.system_surface_container_low_dark,
                ),
                surfaceContainer = c(
                    android.R.color.system_surface_container_light,
                    android.R.color.system_surface_container_dark,
                ),
                surfaceHigh = c(
                    android.R.color.system_surface_container_high_light,
                    android.R.color.system_surface_container_high_dark,
                ),
                surfaceHighest = c(
                    android.R.color.system_surface_container_highest_light,
                    android.R.color.system_surface_container_highest_dark,
                ),
                onSurface = c(android.R.color.system_on_surface_light, android.R.color.system_on_surface_dark),
                onSurfaceVariant = c(
                    android.R.color.system_on_surface_variant_light,
                    android.R.color.system_on_surface_variant_dark,
                ),
                outline = c(android.R.color.system_outline_light, android.R.color.system_outline_dark),
                outlineVariant = c(
                    android.R.color.system_outline_variant_light,
                    android.R.color.system_outline_variant_dark,
                ),
                error = c(android.R.color.system_error_light, android.R.color.system_error_dark),
                errorContainer = c(
                    android.R.color.system_error_container_light,
                    android.R.color.system_error_container_dark,
                ),
                onErrorContainer = c(
                    android.R.color.system_on_error_container_light,
                    android.R.color.system_on_error_container_dark,
                ),
            )
        }
    }
}

class TextStyle(val size: Float, val line: Float, val weight: Int)

object Look {
    const val OPAQUE = 0xFF000000.toInt()
    const val RGB = 0xFFFFFF
    const val BRAND = 0xFF10B981.toInt()

    // Material 3 type scale, with the heavier weights of the expressive update for titles.
    val DISPLAY = TextStyle(36f, 44f, 500)
    val HEADLINE = TextStyle(24f, 32f, 500)
    val TITLE = TextStyle(18f, 24f, 500)
    val BODY_LARGE = TextStyle(16f, 24f, 400)
    val BODY = TextStyle(14f, 20f, 400)
    val LABEL = TextStyle(14f, 20f, 500)
    val LABEL_LARGE = TextStyle(16f, 24f, 500)
    val CAPTION = TextStyle(12f, 16f, 400)

    const val SPACE_XXS = 2f
    const val SPACE_XS = 4f
    const val SPACE_SM = 8f
    const val SPACE_MD = 12f
    const val SPACE_LG = 16f
    const val SPACE_XL = 20f
    const val SPACE_XXL = 24f
    const val SPACE_HUGE = 48f

    // Shapes: a group of list rows has large outer and small inner corners, two points apart.
    const val RADIUS_INNER = 4f
    const val RADIUS_GROUP = 24f
    const val RADIUS_CARD = 28f
    const val RADIUS_FULL = 100f
    const val GROUP_GAP = 2f

    const val CONTROL = 40f
    const val CONTROL_LARGE = 56f
    const val ROW_MIN = 72f
    const val ICON_CIRCLE = 40f
    const val ICON = 24f
    const val ICON_SMALL = 18f
    const val SWITCH_WIDTH = 52f
    const val SWITCH_HEIGHT = 32f
    const val SWITCH_THUMB_OFF = 16f
    const val SWITCH_THUMB_ON = 24f
    const val SWITCH_THUMB_PRESSED = 28f
    const val SWITCH_OUTLINE = 2f
    const val SLIDER_TRACK = 16f
    const val SLIDER_HANDLE_WIDTH = 4f
    const val SLIDER_HANDLE_HEIGHT = 44f
    const val STATUS_DOT = 8f
    const val STROKE = 1f

    const val FAST = 150L
    const val BASE = 300L
    const val SLOW = 400L
    const val PAGE_SHIFT = 8f

    val standard = PathInterpolator(0.2f, 0f, 0f, 1f)
    val emphasized = PathInterpolator(0.05f, 0.7f, 0.1f, 1f)
    val exit = PathInterpolator(0.3f, 0f, 1f, 1f)

    // The fast spatial spring of Material 3 Expressive, as a curve that slightly overshoots.
    val spring = PathInterpolator(0.3f, 1.4f, 0.5f, 1f)

    fun dark(context: Context): Boolean {
        val night = context.resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK
        return night == Configuration.UI_MODE_NIGHT_YES
    }

    fun palette(context: Context): Palette {
        val dark = dark(context)
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            Palette.dynamic(context, dark)
        } else {
            Palette.green(dark)
        }
    }

    fun reducedMotion(): Boolean = !ValueAnimator.areAnimatorsEnabled()

    fun alpha(color: Int, amount: Float): Int = ((amount * 255).roundToInt() shl 24) or (color and RGB)

    fun blend(from: Int, to: Int, amount: Float): Int {
        val t = amount.coerceIn(0f, 1f)
        fun channel(shift: Int): Int {
            val a = (from ushr shift) and 0xFF
            val b = (to ushr shift) and 0xFF
            return ((a + (b - a) * t).roundToInt() and 0xFF) shl shift
        }
        return channel(24) or channel(16) or channel(8) or channel(0)
    }

    fun dp(context: Context, value: Float): Float =
        TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, context.resources.displayMetrics)

    fun px(context: Context, value: Float): Int = dp(context, value).roundToInt()

    fun style(view: TextView, style: TextStyle) {
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, style.size)
        view.typeface = Typeface.create(Typeface.DEFAULT, style.weight, false)
        view.setLineHeight(
            TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, style.line, view.resources.displayMetrics)
                .roundToInt(),
        )
        view.includeFontPadding = false
    }
}
