package com.gamescrcpy.companion

import android.Manifest
import android.content.ActivityNotFoundException
import android.content.Intent
import android.content.pm.PackageInfo
import android.content.pm.PackageManager
import android.graphics.drawable.GradientDrawable
import android.net.Uri
import android.os.Build
import android.provider.Settings
import android.text.format.DateUtils
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.InputMethodManager
import android.widget.Chronometer
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.TextView
import com.gamescrcpy.protocol.Enums.CompanionItem
import kotlin.math.roundToInt

// The companion's one page, laid out like Settings on Android 17: the connection on top, then grouped lists for
// what the companion does, the computers it knows, the permissions it needs and the app itself.
class HomePage(private val shell: Shell) {
    private class Switched(val feature: Feature, val row: ListRow, val toggle: SwitchView, val options: List<View>)

    private enum class Permission(val title: Int, val about: Int, val icon: Int) {
        OVERLAY(R.string.perm_overlay, R.string.perm_overlay_about, R.drawable.ic_layers),
        NOTIFICATIONS(R.string.perm_notifications, R.string.perm_notifications_about, R.drawable.ic_bell),
        CAMERA(R.string.perm_camera, R.string.perm_camera_about, R.drawable.ic_camera),
        INPUT_METHOD(R.string.perm_input_method, R.string.perm_input_method_about, R.drawable.ic_keyboard),
        CAPTURE(R.string.perm_capture, R.string.perm_capture_about, R.drawable.ic_capture),
    }

    private val ui = shell.ui
    private val palette = ui.palette
    private val context = ui.context
    private val content = ui.column()

    val view: ScrollView = ScrollView(context).apply {
        isFillViewport = true
        isVerticalScrollBarEnabled = false
        addView(content, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
    }

    private val hero = ui.card(palette.item)
    private val dot = View(context)
    private val state = ui.text(Look.LABEL, palette.onSurfaceVariant)
    private val name = ui.text(Look.HEADLINE, palette.onSurface)
    private val link = ui.text(Look.BODY, palette.onSurfaceVariant)
    private val clock = Chronometer(context)
    private val note = ui.text(Look.BODY, palette.onSurfaceVariant)
    private val older = ui.text(Look.BODY, palette.warning, R.string.home_older_computer)
    private val scan = ui.button(ButtonKind.FILLED, R.string.lan_scan, R.drawable.ic_scan, large = true)
    private val again = ui.button(ButtonKind.TONAL, R.string.lan_connect, large = true)
    private val stop = ui.button(ButtonKind.TEXT, R.string.notification_stop)

    private val missingGroup = ListGroup(ui)
    private val missing = ui.listRow(R.drawable.ic_shield, Hue.ERROR, R.string.home_permissions_title)
    private val features = ListGroup(ui)
    private val switched = ArrayList<Switched>()
    private val opacityValue = ui.text(Look.LABEL, palette.onSurfaceVariant)
    private var dragging = false
    private lateinit var opacity: SeekBar
    private val computers = ListGroup(ui)
    private val permissionsHeader = ui.header(R.string.permissions_title)
    private val permissions = ListGroup(ui)
    private val permissionRows = HashMap<Permission, ListRow>()
    private val about = ListGroup(ui)
    private val version = ui.listRow(R.drawable.ic_info, Hue.SECONDARY, R.string.app_name, R.string.about_text)

    init {
        val title = ui.column(Look.SPACE_XS)
        title.setPaddingRelative(ui.px(Look.SPACE_SM), ui.px(Look.SPACE_HUGE), ui.px(Look.SPACE_SM), ui.px(Look.SPACE_XXL))
        title.addView(ui.text(Look.DISPLAY, palette.onSurface, R.string.app_name))
        title.addView(ui.text(Look.BODY_LARGE, palette.onSurfaceVariant, R.string.home_subtitle))
        content.addView(title, wide())

        buildHero()
        content.addView(hero, wide())

        missing.view.isClickable = true
        missing.view.setOnClickListener { view.smoothScrollTo(0, permissionsHeader.top) }
        missing.trailing.addView(ui.chevron())
        missingGroup.add(missing.view)
        missingGroup.reshape()
        content.addView(missingGroup.view, wide().apply { topMargin = ui.px(Look.SPACE_LG) })

        content.addView(ui.header(R.string.features_title), wide())
        Feature.entries.forEach { buildFeature(it) }
        features.reshape()
        content.addView(features.view, wide())
        content.addView(ui.caption(R.string.features_synced), wide())

        content.addView(ui.header(R.string.computers_title), wide())
        content.addView(computers.view, wide())

        content.addView(permissionsHeader, wide())
        Permission.entries.forEach { buildPermission(it) }
        permissions.reshape()
        content.addView(permissions.view, wide())

        content.addView(ui.header(R.string.about_title), wide())
        buildAbout()
        about.reshape()
        content.addView(about.view, wide())
        inset(0, 0)
    }

    fun inset(top: Int, bottom: Int) {
        val side = ui.px(Look.SPACE_LG)
        content.setPaddingRelative(side, top, side, bottom + ui.px(Look.SPACE_HUGE))
    }

    private fun buildHero() {
        dot.background = GradientDrawable().apply { shape = GradientDrawable.OVAL }
        val head = ui.row(Look.SPACE_SM)
        head.addView(dot, LinearLayout.LayoutParams(ui.px(Look.STATUS_DOT), ui.px(Look.STATUS_DOT)))
        head.addView(state)
        hero.addView(head)
        hero.addView(name)
        Look.style(clock, Look.BODY)
        clock.format = context.getString(R.string.home_connected_for)
        val facts = ui.row(Look.SPACE_MD)
        facts.addView(link)
        facts.addView(clock)
        hero.addView(facts)
        hero.addView(note)
        hero.addView(older)
        val buttons = ui.column(Look.SPACE_SM)
        buttons.addView(scan, wide())
        buttons.addView(again, wide())
        buttons.addView(stop, LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT).apply { gravity = Gravity.END })
        hero.addView(buttons, wide().apply { topMargin = ui.px(Look.SPACE_SM) })
        scan.setOnClickListener { shell.scan() }
        again.setOnClickListener { shell.connectNow() }
        stop.setOnClickListener { shell.stopCompanion() }
    }

    private fun buildFeature(feature: Feature) {
        val row = ui.listRow(feature.icon, feature.hue, feature.title, feature.short)
        val label = context.getString(feature.title)
        val toggle = ui.switch(shell.sync.values.valueOf(feature.item) != 0, label) { on ->
            shell.sync.change(feature.item, if (on) 1 else 0)
            refresh()
        }
        row.trailing.addView(toggle)
        row.view.isClickable = true
        row.view.setOnClickListener { toggle.performClick() }
        row.status.setText(R.string.home_not_sent)
        row.status.setTextColor(palette.warning)
        features.add(row.view)
        val options = when (feature) {
            Feature.CURSOR -> listOf(
                option(R.string.option_cursor_size, ui.segments(
                    listOf(R.string.option_small, R.string.option_medium, R.string.option_large),
                    shell.options.cursorSize.ordinal,
                ) { shell.options.chooseCursorSize(CursorSize.entries[it]) }),
                option(R.string.option_cursor_style, ui.segments(
                    listOf(R.string.option_arrow, R.string.option_dot),
                    shell.options.cursorStyle.ordinal,
                ) { shell.options.chooseCursorStyle(CursorStyle.entries[it]) }),
            )
            Feature.HINTS -> listOf(option(R.string.option_opacity, opacityLine()))
            Feature.KEYBOARD -> listOf(
                option(R.string.option_hint_bar, ui.segments(
                    listOf(R.string.option_bottom, R.string.option_top),
                    shell.options.imeBar.ordinal,
                ) { shell.options.chooseImeBar(ImeBar.entries[it]) }, R.string.option_hint_bar_note),
            )
            Feature.AUTO_CONNECT -> emptyList()
        }
        options.forEach { features.add(it) }
        switched += Switched(feature, row, toggle, options)
    }

    // An option of a feature, shown under it while it is on, its text in line with the feature's title.
    private fun option(label: Int, control: View, note: Int = 0): View {
        val block = ui.column(Look.SPACE_SM)
        val start = ui.px(Look.SPACE_LG + Look.ICON_CIRCLE + Look.SPACE_LG)
        block.setPaddingRelative(start, ui.px(Look.SPACE_MD), ui.px(Look.SPACE_LG), ui.px(Look.SPACE_LG))
        block.addView(ui.text(Look.LABEL, palette.onSurface, label))
        block.addView(control, wide())
        if (note != 0) {
            block.addView(ui.text(Look.CAPTION, palette.onSurfaceVariant, note))
        }
        return block
    }

    private fun opacityLine(): View {
        opacity = ui.slider(
            SyncValues.MIN_OPACITY,
            SyncValues.MAX_OPACITY,
            shell.sync.values.hintOpacity,
            { value ->
                dragging = true
                showOpacity(value)
            },
            { value ->
                dragging = false
                val snapped = ((value / OPACITY_STEP.toFloat()).roundToInt() * OPACITY_STEP)
                    .coerceIn(SyncValues.MIN_OPACITY, SyncValues.MAX_OPACITY)
                showOpacity(snapped)
                shell.sync.change(CompanionItem.HINT_OPACITY, snapped)
                refresh()
            },
        )
        val line = ui.row(Look.SPACE_SM)
        line.addView(opacity, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        opacityValue.minWidth = ui.px(VALUE_WIDTH)
        opacityValue.textAlignment = View.TEXT_ALIGNMENT_VIEW_END
        line.addView(opacityValue)
        showOpacity(shell.sync.values.hintOpacity)
        return line
    }

    private fun showOpacity(value: Int) {
        opacityValue.text = context.getString(R.string.option_percent, value)
    }

    private fun buildPermission(permission: Permission) {
        val row = ui.listRow(permission.icon, Hue.SECONDARY, permission.title, permission.about)
        row.status.visibility = View.VISIBLE
        row.trailing.addView(ui.chevron())
        row.view.isClickable = true
        row.view.setOnClickListener { open(permission) }
        permissions.add(row.view)
        permissionRows[permission] = row
    }

    private fun buildAbout() {
        about.add(version.view)
        fun entry(icon: Int, title: Int, action: () -> Unit) {
            val row = ui.listRow(icon, Hue.SECONDARY, title)
            row.trailing.addView(ui.chevron())
            row.view.isClickable = true
            row.view.setOnClickListener { action() }
            about.add(row.view)
        }
        entry(R.drawable.ic_help, R.string.about_help) { browse(context.getString(R.string.about_help_link)) }
        entry(R.drawable.ic_shield, R.string.about_privacy) {
            ui.inform(R.string.about_privacy, ui.text(Look.BODY, palette.onSurfaceVariant, R.string.about_privacy_text))
        }
        entry(R.drawable.ic_license, R.string.about_licenses) {
            val list = ui.column(Look.SPACE_MD)
            list.addView(ui.text(Look.BODY, palette.onSurfaceVariant, R.string.about_licenses_text))
            for ((library, license) in LICENSES) {
                val line = ui.column(Look.SPACE_XXS)
                line.addView(ui.text(Look.LABEL, palette.onSurface, library))
                line.addView(ui.text(Look.BODY, palette.onSurfaceVariant, license))
                list.addView(line)
            }
            ui.inform(R.string.about_licenses, list)
        }
    }

    fun refresh() {
        refreshHero()
        refreshFeatures()
        refreshComputers()
        refreshPermissions()
        packageInfo()?.let { info ->
            version.title.text = context.getString(R.string.about_version, info.versionName.orEmpty(), info.longVersionCode)
        }
    }

    private fun refreshHero() {
        val sync = shell.sync
        val computer = sync.connected
        val connected = CompanionState.connected
        val lan = CompanionState.lan
        val searching = lan == LanState.SEARCHING || lan == LanState.PAIRING
        val waiting = !connected && (CompanionState.listening || searching)
        hero.background = ui.rounded(if (connected) palette.primaryContainer else palette.item, Look.RADIUS_CARD)
        val strong = if (connected) palette.onPrimaryContainer else palette.onSurface
        val soft = if (connected) palette.onPrimaryContainer else palette.onSurfaceVariant
        name.setTextColor(strong)
        link.setTextColor(soft)
        clock.setTextColor(soft)
        note.setTextColor(soft)
        val dotColor = when {
            connected -> palette.success
            waiting -> palette.warning
            lan == LanState.FAILED -> palette.error
            else -> palette.outline
        }
        (dot.background as? GradientDrawable)?.setColor(if (connected) palette.onPrimaryContainer else dotColor)
        state.setTextColor(if (connected) palette.onPrimaryContainer else dotColor)
        state.setText(
            when {
                connected -> R.string.state_connected
                searching -> R.string.state_searching
                waiting -> R.string.state_waiting
                else -> R.string.state_not_connected
            },
        )
        when {
            connected && computer != null -> name.text = Labels.name(context, computer)
            connected -> name.setText(R.string.home_computer_generic)
            searching -> name.setText(R.string.home_looking)
            waiting -> name.setText(R.string.home_waiting_title)
            else -> name.setText(R.string.home_idle_title)
        }
        link.setText(Labels.link(computer, CompanionState.lanConnected && !CompanionState.localConnected))
        link.visibility = if (connected) View.VISIBLE else View.GONE
        if (connected) {
            clock.base = CompanionState.connectedClock
            clock.visibility = View.VISIBLE
            clock.start()
        } else {
            clock.stop()
            clock.visibility = View.GONE
        }
        val explanation = when {
            connected -> 0
            searching -> R.string.lan_searching
            lan == LanState.FAILED -> R.string.lan_failed
            CompanionState.listening -> R.string.home_waiting
            else -> R.string.home_idle
        }
        note.visibility = if (explanation == 0) View.GONE else View.VISIBLE
        if (explanation != 0) {
            note.setText(explanation)
        }
        older.visibility = if (connected && computer == null) View.VISIBLE else View.GONE
        val paired = PairingStore(context).all().isNotEmpty()
        scan.visibility = if (connected) View.GONE else View.VISIBLE
        again.visibility = if (paired && !CompanionState.lanConnected && !searching) View.VISIBLE else View.GONE
        stop.visibility = if (connected || waiting) View.VISIBLE else View.GONE
    }

    private fun refreshFeatures() {
        val values = shell.sync.values
        val unsent = shell.sync.pendingItems
        val delivering = shell.sync.connected != null
        for (entry in switched) {
            val on = values.valueOf(entry.feature.item) != 0
            if (entry.toggle.isChecked() != on) {
                entry.toggle.set(on, animate = true)
            }
            entry.row.status.visibility =
                if (entry.feature.item in unsent && !delivering) View.VISIBLE else View.GONE
            entry.options.forEach { it.visibility = if (on) View.VISIBLE else View.GONE }
        }
        if (::opacity.isInitialized && !dragging && opacity.progress != values.hintOpacity) {
            opacity.progress = values.hintOpacity
            showOpacity(values.hintOpacity)
        }
        features.reshape()
    }

    private fun refreshComputers() {
        computers.clear()
        val pairings = PairingStore(context).all()
        val known = shell.sync.known
        val connected = shell.sync.connected
        for (computer in known) {
            val pairing = pairings.firstOrNull { it.pc.isNotEmpty() && it.pc == computer.pc }
            computers.add(computerRow(computer, pairing, computer.id == connected?.id))
        }
        for (pairing in pairings) {
            if (known.none { it.pc == pairing.pc }) {
                computers.add(computerRow(null, pairing, false))
            }
        }
        if (computers.view.childCount == 0) {
            val empty = ui.listRow(R.drawable.ic_computer, Hue.SECONDARY, R.string.computers_empty_title,
                R.string.computers_empty_text)
            computers.add(empty.view)
        }
        computers.reshape()
    }

    private fun computerRow(computer: KnownComputer?, pairing: Pairing?, live: Boolean): View {
        val row = ui.listRow(R.drawable.ic_computer, if (live) Hue.PRIMARY else Hue.SECONDARY, 0)
        val title = if (computer != null) Labels.name(context, computer) else context.getString(R.string.computers_lan_only)
        row.title.text = title
        val seen = when {
            live -> null
            computer != null -> context.getString(
                R.string.computers_last_seen,
                DateUtils.getRelativeTimeSpanString(computer.lastSeen, System.currentTimeMillis(),
                    DateUtils.MINUTE_IN_MILLIS),
            )
            pairing != null -> context.getString(R.string.computers_last_address, "${pairing.host}:${pairing.port}")
            else -> null
        }
        val how = computer?.let { context.getString(Labels.link(it, false)) }
        val support = listOfNotNull(seen, how).joinToString(context.getString(R.string.fact_separator))
        row.support.text = support
        row.support.visibility = if (support.isEmpty()) View.GONE else View.VISIBLE
        when {
            live -> status(row, R.string.computers_connected_now, palette.success)
            pairing != null -> status(row, R.string.computers_paired, palette.onSurfaceVariant)
        }
        row.trailing.addView(ui.iconButton(R.drawable.ic_delete, context.getString(R.string.computers_remove)) {
            ui.confirm(context.getString(R.string.computers_remove_title, title), R.string.computers_remove_text,
                R.string.computers_remove) {
                computer?.let { known -> shell.sync.forget(known.id) }
                pairing?.let { paired -> PairingStore(context).forget(paired.pc) }
                refresh()
            }
        })
        return row.view
    }

    private fun status(row: ListRow, text: Int, color: Int) {
        row.status.setText(text)
        row.status.setTextColor(color)
        row.status.visibility = View.VISIBLE
    }

    private fun refreshPermissions() {
        for ((permission, row) in permissionRows) {
            val (good, text) = when (permission) {
                Permission.OVERLAY -> granted(PhonePermissions.overlay(context))
                Permission.NOTIFICATIONS -> granted(PhonePermissions.notifications(context))
                Permission.CAMERA -> if (PhonePermissions.camera(context)) {
                    true to R.string.perm_allowed
                } else {
                    null to R.string.perm_when_scanning
                }
                Permission.INPUT_METHOD -> when (PhonePermissions.ime(context)) {
                    ImeStatus.CHOSEN -> true to R.string.perm_ime_chosen
                    ImeStatus.ENABLED -> null to R.string.perm_ime_enabled
                    ImeStatus.OFF -> null to R.string.perm_ime_off
                }
                Permission.CAPTURE -> if (PhonePermissions.capture(context)) {
                    true to R.string.perm_capture_ready
                } else {
                    false to R.string.perm_capture_blocked
                }
            }
            status(row, text, when (good) {
                true -> palette.success
                false -> palette.error
                null -> palette.onSurfaceVariant
            })
        }
        val lacking = PhonePermissions.missing(context)
        missingGroup.view.visibility = if (lacking.isEmpty()) View.GONE else View.VISIBLE
        missing.support.visibility = View.VISIBLE
        missing.support.text = lacking.joinToString(context.getString(R.string.list_separator)) { context.getString(it) }
    }

    private fun granted(on: Boolean): Pair<Boolean?, Int> =
        if (on) true to R.string.perm_allowed else false to R.string.perm_missing

    private fun open(permission: Permission) {
        val activity = shell.activity
        val app = Uri.fromParts("package", activity.packageName, null)
        when (permission) {
            Permission.OVERLAY, Permission.CAPTURE -> launch(Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, app))
            Permission.NOTIFICATIONS -> {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
                    !PhonePermissions.notifications(activity) &&
                    shell.ask(Manifest.permission.POST_NOTIFICATIONS)
                ) {
                    return
                }
                launch(Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS)
                    .putExtra(Settings.EXTRA_APP_PACKAGE, activity.packageName))
            }
            Permission.CAMERA -> {
                if (!PhonePermissions.camera(activity) && shell.ask(Manifest.permission.CAMERA)) {
                    return
                }
                launch(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, app))
            }
            Permission.INPUT_METHOD -> if (PhonePermissions.ime(activity) == ImeStatus.ENABLED) {
                context.getSystemService(InputMethodManager::class.java)?.showInputMethodPicker()
            } else {
                launch(Intent(Settings.ACTION_INPUT_METHOD_SETTINGS))
            }
        }
    }

    private fun launch(intent: Intent) {
        try {
            shell.activity.startActivity(intent)
        } catch (e: ActivityNotFoundException) {
            shell.activity.startActivity(Intent(Settings.ACTION_SETTINGS))
        }
    }

    private fun browse(link: String) {
        try {
            shell.activity.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(link)))
        } catch (e: ActivityNotFoundException) {
            return
        }
    }

    private fun packageInfo(): PackageInfo? = try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            context.packageManager.getPackageInfo(context.packageName, PackageManager.PackageInfoFlags.of(0))
        } else {
            legacyPackageInfo()
        }
    } catch (e: PackageManager.NameNotFoundException) {
        null
    }

    @Suppress("DEPRECATION")
    private fun legacyPackageInfo(): PackageInfo = context.packageManager.getPackageInfo(context.packageName, 0)

    private fun wide(): LinearLayout.LayoutParams =
        LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)

    private companion object {
        const val OPACITY_STEP = 5
        const val VALUE_WIDTH = 48f
        val LICENSES = listOf(
            R.string.license_gamescrcpy to R.string.license_apache,
            R.string.license_kotlin to R.string.license_apache,
            R.string.license_camerax to R.string.license_apache,
            R.string.license_lifecycle to R.string.license_apache,
            R.string.license_zxing to R.string.license_apache,
        )
    }
}
