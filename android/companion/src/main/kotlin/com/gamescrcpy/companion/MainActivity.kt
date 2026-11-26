package com.gamescrcpy.companion

import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.FrameLayout

class MainActivity : Activity(), Shell {
    override val activity: Activity
        get() = this
    override lateinit var ui: Ui
    override val sync: SettingsSync by lazy { SettingsSync.of(this) }
    override val options: PhoneOptions by lazy { PhoneOptions.of(this) }

    private lateinit var page: HomePage
    private val asked = HashSet<String>()
    private val stateListener = CompanionState.Listener { runOnUiThread { refresh() } }
    private val syncListener = SettingsSync.Listener { runOnUiThread { refresh() } }
    private val optionsListener = PhoneOptions.Listener { runOnUiThread { refresh() } }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        build()
        if (savedInstanceState == null && sync.values.autoConnect && !CompanionState.lanConnected &&
            CompanionState.lan != LanState.SEARCHING && CompanionState.lan != LanState.PAIRING &&
            PairingStore(this).all().isNotEmpty()
        ) {
            connectNow()
        }
    }

    override fun onResume() {
        super.onResume()
        CompanionState.addListener(stateListener)
        sync.addListener(syncListener)
        options.addListener(optionsListener)
        refresh()
    }

    override fun onPause() {
        CompanionState.removeListener(stateListener)
        sync.removeListener(syncListener)
        options.removeListener(optionsListener)
        super.onPause()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            refresh()
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        refresh()
    }

    override fun scan() {
        startActivity(Intent(this, ScanActivity::class.java))
    }

    override fun connectNow() {
        startForegroundService(Intent(this, CompanionService::class.java).setAction(CompanionService.ACTION_CONNECT))
    }

    override fun stopCompanion() {
        startService(Intent(this, CompanionService::class.java).setAction(CompanionService.ACTION_STOP))
    }

    override fun ask(permission: String): Boolean {
        if (checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED || !asked.add(permission)) {
            return false
        }
        requestPermissions(arrayOf(permission), REQUEST_PERMISSION)
        return true
    }

    private fun build() {
        ui = Ui(this, Look.palette(this))
        page = HomePage(this)
        val root = FrameLayout(this)
        root.setBackgroundColor(ui.palette.page)
        window.decorView.setBackgroundColor(ui.palette.page)
        root.addView(page.view, FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT))
        // Behind the status bar the page colour stays, so the list does not run under the clock.
        val scrim = View(this).apply { setBackgroundColor(ui.palette.page) }
        root.addView(scrim, FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, Gravity.TOP))
        root.setOnApplyWindowInsetsListener { view, insets ->
            val bars = barsOf(insets)
            view.setPadding(bars[0], 0, bars[2], 0)
            page.inset(bars[1], bars[3])
            scrim.layoutParams = (scrim.layoutParams as FrameLayout.LayoutParams).apply { height = bars[1] }
            insets
        }
        page.view.clipToPadding = false
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false)
        }
        setContentView(root)
        lightBars(!ui.palette.dark)
    }

    private fun refresh() {
        if (::page.isInitialized) {
            page.refresh()
        }
    }

    @Suppress("DEPRECATION")
    private fun lightBars(light: Boolean) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            val mask = android.view.WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS or
                android.view.WindowInsetsController.APPEARANCE_LIGHT_NAVIGATION_BARS
            window.insetsController?.setSystemBarsAppearance(if (light) mask else 0, mask)
        }
    }

    private fun barsOf(insets: WindowInsets): IntArray {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            val bars = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
            return intArrayOf(bars.left, bars.top, bars.right, bars.bottom)
        }
        return legacyBarsOf(insets)
    }

    @Suppress("DEPRECATION")
    private fun legacyBarsOf(insets: WindowInsets): IntArray = intArrayOf(
        insets.systemWindowInsetLeft,
        insets.systemWindowInsetTop,
        insets.systemWindowInsetRight,
        insets.systemWindowInsetBottom,
    )

    private companion object {
        const val REQUEST_PERMISSION = 7
    }
}
