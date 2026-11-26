package com.gamescrcpy.companion

import android.app.Activity
import android.content.Context
import com.gamescrcpy.protocol.Enums.CompanionItem
import com.gamescrcpy.protocol.Enums.ComputerLink

interface Shell {
    val activity: Activity
    val ui: Ui
    val sync: SettingsSync
    val options: PhoneOptions

    fun scan()

    fun connectNow()

    fun stopCompanion()

    fun ask(permission: String): Boolean
}

enum class Feature(
    val item: CompanionItem,
    val title: Int,
    val short: Int,
    val icon: Int,
    val hue: Hue,
) {
    CURSOR(CompanionItem.CURSOR, R.string.feature_cursor, R.string.feature_cursor_short, R.drawable.ic_cursor, Hue.PRIMARY),
    HINTS(CompanionItem.HINTS, R.string.feature_hints, R.string.feature_hints_short, R.drawable.ic_hints, Hue.TERTIARY),
    KEYBOARD(
        CompanionItem.IME,
        R.string.feature_keyboard,
        R.string.feature_keyboard_short,
        R.drawable.ic_keyboard,
        Hue.SECONDARY,
    ),
    AUTO_CONNECT(
        CompanionItem.AUTO_CONNECT,
        R.string.feature_auto_connect,
        R.string.feature_auto_connect_short,
        R.drawable.ic_wifi,
        Hue.PRIMARY,
    ),
}

object Labels {
    fun link(computer: KnownComputer?, overLan: Boolean): Int = when {
        computer?.link == ComputerLink.USB -> R.string.link_usb
        computer?.link == ComputerLink.WIRELESS -> R.string.link_wireless
        computer?.link == ComputerLink.LAN || overLan -> R.string.link_lan
        else -> R.string.link_adb
    }

    fun name(context: Context, computer: KnownComputer?): String =
        computer?.name?.takeIf { it.isNotBlank() } ?: context.getString(R.string.computer_unnamed)
}
