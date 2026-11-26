package com.gamescrcpy.companion

import android.animation.ValueAnimator
import android.app.Dialog
import android.content.Context
import android.content.res.ColorStateList
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.drawable.ClipDrawable
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.Drawable
import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.LayerDrawable
import android.graphics.drawable.RippleDrawable
import android.text.TextUtils
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.view.Window
import android.view.accessibility.AccessibilityEvent
import android.view.accessibility.AccessibilityNodeInfo
import android.widget.Checkable
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.Switch
import android.widget.TextView

enum class ButtonKind { FILLED, TONAL, TEXT, DANGER }

// The tonal circle behind a row's icon.
enum class Hue { PRIMARY, SECONDARY, TERTIARY, ERROR }

// A Material 3 switch: an outlined track with a small thumb when off; a filled track and a larger thumb with a
// check mark when on. The thumb grows while pressed and moves on a spring.
class SwitchView(context: Context, private val palette: Palette) : View(context), Checkable {
    var onToggle: ((Boolean) -> Unit)? = null

    private var on = false
    private var position = 0f
    private var pressedAmount = 0f
    private var animator: ValueAnimator? = null
    private var pressAnimator: ValueAnimator? = null
    private val trackWidth = Look.dp(context, Look.SWITCH_WIDTH)
    private val trackHeight = Look.dp(context, Look.SWITCH_HEIGHT)
    private val thumbOff = Look.dp(context, Look.SWITCH_THUMB_OFF)
    private val thumbOn = Look.dp(context, Look.SWITCH_THUMB_ON)
    private val thumbPressed = Look.dp(context, Look.SWITCH_THUMB_PRESSED)
    private val outlineWidth = Look.dp(context, Look.SWITCH_OUTLINE)
    private val touchWidth = Look.px(context, Look.SWITCH_WIDTH + Look.SPACE_SM)
    private val touchHeight = Look.px(context, Look.CONTROL_LARGE)
    private val rect = RectF()
    private val check = Path()
    private val fill = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.FILL }
    private val outline = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.STROKE }
    private val thumb = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.FILL }
    private val mark = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeCap = Paint.Cap.ROUND
        strokeJoin = Paint.Join.ROUND
        strokeWidth = Look.dp(context, CHECK_STROKE)
    }

    init {
        isClickable = true
        isFocusable = true
        setOnClickListener {
            toggle()
            onToggle?.invoke(on)
        }
    }

    override fun isChecked(): Boolean = on

    override fun setChecked(checked: Boolean) {
        set(checked, animate = false)
    }

    override fun toggle() {
        set(!on, animate = true)
    }

    fun set(checked: Boolean, animate: Boolean) {
        if (checked == on && animator == null) {
            return
        }
        on = checked
        animator?.cancel()
        animator = null
        val target = if (checked) 1f else 0f
        if (!animate || Look.reducedMotion() || !isAttachedToWindow) {
            position = target
            invalidate()
            return
        }
        animator = ValueAnimator.ofFloat(position, target).apply {
            duration = Look.BASE
            interpolator = Look.spring
            addUpdateListener {
                position = it.animatedValue as Float
                invalidate()
            }
            start()
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> press(1f)
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> press(0f)
        }
        return super.onTouchEvent(event)
    }

    private fun press(target: Float) {
        pressAnimator?.cancel()
        if (Look.reducedMotion()) {
            pressedAmount = target
            invalidate()
            return
        }
        pressAnimator = ValueAnimator.ofFloat(pressedAmount, target).apply {
            duration = Look.FAST
            interpolator = Look.standard
            addUpdateListener {
                pressedAmount = it.animatedValue as Float
                invalidate()
            }
            start()
        }
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        setMeasuredDimension(resolveSize(touchWidth, widthMeasureSpec), resolveSize(touchHeight, heightMeasureSpec))
    }

    override fun onDraw(canvas: Canvas) {
        val left = (width - trackWidth) / 2
        val top = (height - trackHeight) / 2
        val radius = trackHeight / 2
        val p = position.coerceIn(0f, 1f)
        rect.set(left, top, left + trackWidth, top + trackHeight)
        fill.color = Look.blend(palette.surfaceHighest, palette.primary, p)
        canvas.drawRoundRect(rect, radius, radius, fill)
        if (p < 1f) {
            outline.strokeWidth = outlineWidth
            outline.color = Look.alpha(palette.outline, 1f - p)
            rect.inset(outlineWidth / 2, outlineWidth / 2)
            canvas.drawRoundRect(rect, radius, radius, outline)
        }
        val resting = thumbOff + (thumbOn - thumbOff) * p
        val size = resting + (thumbPressed - resting) * pressedAmount
        val travelStart = left + radius
        val travelEnd = left + trackWidth - radius
        val cx = travelStart + (travelEnd - travelStart) * position
        val cy = top + radius
        thumb.color = Look.blend(palette.outline, palette.onPrimary, p)
        canvas.drawCircle(cx, cy, size / 2, thumb)
        if (p > CHECK_FROM) {
            val unit = size / CHECK_UNITS
            check.reset()
            check.moveTo(cx - 2.5f * unit, cy)
            check.lineTo(cx - 0.7f * unit, cy + 1.8f * unit)
            check.lineTo(cx + 2.7f * unit, cy - 1.8f * unit)
            mark.color = Look.alpha(palette.primary, (p - CHECK_FROM) / (1f - CHECK_FROM))
            canvas.drawPath(check, mark)
        }
    }

    override fun getAccessibilityClassName(): CharSequence = Switch::class.java.name

    override fun onInitializeAccessibilityNodeInfo(info: AccessibilityNodeInfo) {
        super.onInitializeAccessibilityNodeInfo(info)
        info.isCheckable = true
        info.isChecked = on
    }

    override fun onInitializeAccessibilityEvent(event: AccessibilityEvent) {
        super.onInitializeAccessibilityEvent(event)
        event.isChecked = on
    }

    private companion object {
        const val CHECK_STROKE = 2f
        const val CHECK_UNITS = 8f
        const val CHECK_FROM = 0.6f
    }
}

// One row of a grouped list: a tonal icon circle, a title, an optional coloured status line and a supporting text,
// and a trailing control.
class ListRow(
    val view: LinearLayout,
    val title: TextView,
    val status: TextView,
    val support: TextView,
    val trailing: FrameLayout,
)

// Rows of a Settings list: large corners at the ends of the group, small ones between rows, a small gap between.
class ListGroup(private val ui: Ui) {
    val view: LinearLayout = ui.column(Look.GROUP_GAP)

    fun add(row: View) {
        view.addView(row, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT))
    }

    fun clear() {
        view.removeAllViews()
    }

    // Shapes the visible rows again after rows were added, removed, shown or hidden.
    fun reshape() {
        val shown = (0 until view.childCount).map { view.getChildAt(it) }.filter { it.visibility != View.GONE }
        shown.forEachIndexed { index, row ->
            val first = index == 0
            val last = index == shown.lastIndex
            row.background = ui.itemBackground(first, last, row.isClickable)
        }
    }
}

// A button whose icon stays next to its text however wide the button is.
class ButtonView(context: Context) : TextView(context) {
    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        super.onLayout(changed, left, top, right, bottom)
        val icon = compoundDrawablesRelative[0] ?: return
        val content = icon.bounds.width() + compoundDrawablePadding + paint.measureText(text.toString())
        val side = ((width - content) / 2).toInt().coerceAtLeast(minSide)
        if (side != paddingStart) {
            post { setPaddingRelative(side, paddingTop, side, paddingBottom) }
        }
        gravity = Gravity.START or Gravity.CENTER_VERTICAL
    }

    private val minSide = Look.px(context, Look.SPACE_LG)
}

class Ui(val context: Context, val palette: Palette) {
    fun px(value: Float): Int = Look.px(context, value)

    fun text(style: TextStyle, color: Int, resource: Int = 0): TextView = TextView(context).apply {
        Look.style(this, style)
        setTextColor(color)
        if (resource != 0) {
            setText(resource)
        }
    }

    fun column(gap: Float = 0f): LinearLayout = LinearLayout(context).apply {
        orientation = LinearLayout.VERTICAL
        if (gap > 0f) {
            showDividers = LinearLayout.SHOW_DIVIDER_MIDDLE
            dividerDrawable = spacer(gap)
        }
    }

    fun row(gap: Float = 0f): LinearLayout = LinearLayout(context).apply {
        orientation = LinearLayout.HORIZONTAL
        gravity = Gravity.CENTER_VERTICAL
        if (gap > 0f) {
            showDividers = LinearLayout.SHOW_DIVIDER_MIDDLE
            dividerDrawable = spacer(gap)
        }
    }

    fun spacer(size: Float): Drawable = GradientDrawable().apply {
        setColor(Color.TRANSPARENT)
        setSize(px(size), px(size))
    }

    fun rounded(color: Int, radius: Float, strokeColor: Int = Color.TRANSPARENT): GradientDrawable =
        GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = Look.dp(context, radius)
            setColor(color)
            if (Color.alpha(strokeColor) != 0) {
                setStroke(px(Look.STROKE), strokeColor)
            }
        }

    fun pressable(content: Drawable, radius: Float): Drawable =
        RippleDrawable(ColorStateList.valueOf(palette.press), content, rounded(Color.WHITE, radius))

    fun itemBackground(first: Boolean, last: Boolean, pressable: Boolean): Drawable {
        val big = Look.dp(context, Look.RADIUS_GROUP)
        val small = Look.dp(context, Look.RADIUS_INNER)
        val top = if (first) big else small
        val bottom = if (last) big else small
        val radii = floatArrayOf(top, top, top, top, bottom, bottom, bottom, bottom)
        fun face(color: Int) = GradientDrawable().apply {
            cornerRadii = radii
            setColor(color)
        }
        val surface = face(palette.item)
        return if (pressable) RippleDrawable(ColorStateList.valueOf(palette.press), surface, face(Color.WHITE)) else surface
    }

    fun card(color: Int): LinearLayout = column(Look.SPACE_MD).apply {
        background = rounded(color, Look.RADIUS_CARD)
        val padding = px(Look.SPACE_XXL)
        setPaddingRelative(padding, padding, padding, padding)
    }

    fun header(resource: Int): TextView = text(Look.LABEL, palette.primary, resource).apply {
        setPaddingRelative(px(Look.SPACE_LG), px(Look.SPACE_XXL), px(Look.SPACE_LG), px(Look.SPACE_SM))
        isFocusable = false
        isAccessibilityHeading = true
    }

    fun caption(resource: Int): TextView = text(Look.CAPTION, palette.onSurfaceVariant, resource).apply {
        setPaddingRelative(px(Look.SPACE_LG), px(Look.SPACE_SM), px(Look.SPACE_LG), 0)
    }

    fun button(kind: ButtonKind, resource: Int, icon: Int = 0, large: Boolean = false): TextView =
        ButtonView(context).apply {
            Look.style(this, if (large) Look.LABEL_LARGE else Look.LABEL)
            setTextColor(inkOf(kind))
            setText(resource)
            gravity = Gravity.CENTER
            val height = px(if (large) Look.CONTROL_LARGE else Look.CONTROL)
            minHeight = height
            minimumHeight = height
            val side = px(if (kind == ButtonKind.TEXT) Look.SPACE_MD else Look.SPACE_XXL)
            setPaddingRelative(side, 0, side, 0)
            background = pressable(faceOf(kind), Look.RADIUS_FULL)
            isClickable = true
            isFocusable = true
            maxLines = 1
            ellipsize = TextUtils.TruncateAt.END
            if (icon != 0) {
                lead(this, icon, inkOf(kind))
            }
        }

    fun iconButton(icon: Int, label: CharSequence, onClick: () -> Unit): ImageView = ImageView(context).apply {
        setImageResource(icon)
        imageTintList = ColorStateList.valueOf(palette.onSurfaceVariant)
        val padding = px(Look.SPACE_SM)
        setPadding(padding, padding, padding, padding)
        background = pressable(ColorDrawable(Color.TRANSPARENT), Look.RADIUS_FULL)
        contentDescription = label
        isClickable = true
        isFocusable = true
        setOnClickListener { onClick() }
        layoutParams = FrameLayout.LayoutParams(px(Look.CONTROL), px(Look.CONTROL), Gravity.CENTER)
    }

    fun lead(view: TextView, icon: Int, color: Int) {
        view.setCompoundDrawablesRelativeWithIntrinsicBounds(icon, 0, 0, 0)
        val drawable = view.compoundDrawablesRelative[0]?.mutate() ?: return
        drawable.setTint(color)
        drawable.setBounds(0, 0, px(Look.ICON_SMALL), px(Look.ICON_SMALL))
        view.setCompoundDrawablesRelative(drawable, null, null, null)
        view.compoundDrawablePadding = px(Look.SPACE_SM)
    }

    fun circle(icon: Int, hue: Hue): FrameLayout {
        val (fill, ink) = when (hue) {
            Hue.PRIMARY -> palette.primaryContainer to palette.onPrimaryContainer
            Hue.SECONDARY -> palette.secondaryContainer to palette.onSecondaryContainer
            Hue.TERTIARY -> palette.tertiaryContainer to palette.onTertiaryContainer
            Hue.ERROR -> palette.errorContainer to palette.onErrorContainer
        }
        return FrameLayout(context).apply {
            background = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(fill)
            }
            importantForAccessibility = View.IMPORTANT_FOR_ACCESSIBILITY_NO
            val image = ImageView(context).apply {
                setImageResource(icon)
                imageTintList = ColorStateList.valueOf(ink)
            }
            addView(image, FrameLayout.LayoutParams(px(Look.ICON_SMALL + Look.SPACE_XXS),
                px(Look.ICON_SMALL + Look.SPACE_XXS), Gravity.CENTER))
        }
    }

    fun listRow(icon: Int, hue: Hue, title: Int, support: Int = 0): ListRow {
        val row = row(Look.SPACE_LG).apply {
            minimumHeight = px(Look.ROW_MIN)
            setPaddingRelative(px(Look.SPACE_LG), px(Look.SPACE_MD), px(Look.SPACE_MD), px(Look.SPACE_MD))
        }
        if (icon != 0) {
            row.addView(circle(icon, hue), LinearLayout.LayoutParams(px(Look.ICON_CIRCLE), px(Look.ICON_CIRCLE)))
        }
        val texts = column(Look.SPACE_XXS)
        val heading = text(Look.BODY_LARGE, palette.onSurface, title)
        val status = text(Look.LABEL, palette.onSurfaceVariant).apply { visibility = View.GONE }
        val supporting = text(Look.BODY, palette.onSurfaceVariant, support).apply {
            visibility = if (support == 0) View.GONE else View.VISIBLE
        }
        texts.addView(heading)
        texts.addView(status)
        texts.addView(supporting)
        row.addView(texts, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        val trailing = FrameLayout(context)
        row.addView(trailing, LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT))
        return ListRow(row, heading, status, supporting, trailing)
    }

    fun chevron(): ImageView = ImageView(context).apply {
        setImageResource(R.drawable.ic_chevron)
        imageTintList = ColorStateList.valueOf(palette.onSurfaceVariant)
        importantForAccessibility = View.IMPORTANT_FOR_ACCESSIBILITY_NO
        layoutParams = FrameLayout.LayoutParams(px(Look.ICON), px(Look.ICON), Gravity.CENTER)
    }

    fun switch(checked: Boolean, label: CharSequence, onToggle: (Boolean) -> Unit): SwitchView =
        SwitchView(context, palette).apply {
            setChecked(checked)
            contentDescription = label
            this.onToggle = onToggle
        }

    // A connected button group: the chosen button turns fully round and tonal.
    fun segments(labels: List<Int>, selected: Int, onPick: (Int) -> Unit): LinearLayout {
        val group = row(Look.GROUP_GAP)
        val items = labels.mapIndexed { index, label ->
            text(Look.LABEL, palette.onSurfaceVariant, label).apply {
                gravity = Gravity.CENTER
                minHeight = px(Look.CONTROL)
                isClickable = true
                isFocusable = true
                maxLines = 1
                setPaddingRelative(px(Look.SPACE_MD), 0, px(Look.SPACE_MD), 0)
                group.addView(this, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
            }
        }
        fun shape(index: Int, chosen: Boolean): Drawable {
            val full = Look.dp(context, Look.CONTROL / 2)
            val inner = Look.dp(context, SEGMENT_INNER)
            val start = if (chosen || index == 0) full else inner
            val end = if (chosen || index == items.lastIndex) full else inner
            val rtl = group.layoutDirection == View.LAYOUT_DIRECTION_RTL
            val left = if (rtl) end else start
            val right = if (rtl) start else end
            fun face(color: Int) = GradientDrawable().apply {
                cornerRadii = floatArrayOf(left, left, right, right, right, right, left, left)
                setColor(color)
            }
            val color = if (chosen) palette.secondaryContainer else palette.surfaceHigh
            return RippleDrawable(ColorStateList.valueOf(palette.press), face(color), face(Color.WHITE))
        }
        fun mark(chosen: Int) {
            items.forEachIndexed { index, item ->
                val on = index == chosen
                item.isSelected = on
                item.setTextColor(if (on) palette.onSecondaryContainer else palette.onSurfaceVariant)
                item.background = shape(index, on)
            }
        }
        mark(selected)
        items.forEachIndexed { index, item ->
            item.setOnClickListener {
                mark(index)
                onPick(index)
            }
        }
        return group
    }

    // The expressive slider: a thick track and a narrow upright handle.
    fun slider(minimum: Int, maximum: Int, value: Int, onMove: (Int) -> Unit, onSet: (Int) -> Unit): SeekBar =
        SeekBar(context).apply {
            min = minimum
            max = maximum
            progress = value
            splitTrack = false
            val radius = Look.SLIDER_TRACK / 2
            val track = rounded(palette.secondaryContainer, radius)
            val done = ClipDrawable(rounded(palette.primary, radius), Gravity.START, ClipDrawable.HORIZONTAL)
            progressDrawable = LayerDrawable(arrayOf<Drawable>(track, done)).apply {
                setId(0, android.R.id.background)
                setId(1, android.R.id.progress)
                for (layer in 0..1) {
                    setLayerGravity(layer, Gravity.CENTER_VERTICAL or Gravity.FILL_HORIZONTAL)
                    setLayerHeight(layer, px(Look.SLIDER_TRACK))
                }
            }
            thumb = GradientDrawable().apply {
                setColor(palette.primary)
                cornerRadius = Look.dp(context, Look.SLIDER_HANDLE_WIDTH / 2)
                setSize(px(Look.SLIDER_HANDLE_WIDTH), px(Look.SLIDER_HANDLE_HEIGHT))
            }
            minimumHeight = px(Look.SLIDER_HANDLE_HEIGHT + Look.SPACE_XS)
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                private var touching = false

                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    if (fromUser) {
                        onMove(progress)
                        if (!touching) {
                            onSet(progress)
                        }
                    }
                }

                override fun onStartTrackingTouch(seekBar: SeekBar) {
                    touching = true
                }

                override fun onStopTrackingTouch(seekBar: SeekBar) {
                    touching = false
                    onSet(seekBar.progress)
                }
            })
        }

    fun confirm(title: CharSequence, message: Int, action: Int, onConfirm: () -> Unit) {
        val text = text(Look.BODY, palette.onSurfaceVariant, message)
        dialog(title, text, listOf(
            R.string.action_cancel to (ButtonKind.TEXT to {}),
            action to (ButtonKind.DANGER to onConfirm),
        ))
    }

    fun inform(title: Int, content: View) {
        dialog(context.getString(title), content, listOf(R.string.action_close to (ButtonKind.TEXT to {})))
    }

    private fun dialog(title: CharSequence, content: View, actions: List<Pair<Int, Pair<ButtonKind, () -> Unit>>>) {
        val dialog = Dialog(context)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)
        val body = column(Look.SPACE_LG).apply {
            background = rounded(palette.surfaceHigh, Look.RADIUS_CARD)
            val padding = px(Look.SPACE_XXL)
            setPadding(padding, padding, padding, px(Look.SPACE_LG))
            addView(text(Look.HEADLINE, palette.onSurface).apply { text = title })
            val scroll = ScrollView(context).apply {
                isVerticalScrollBarEnabled = false
                addView(content)
            }
            addView(scroll, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f).apply {
                height = ViewGroup.LayoutParams.WRAP_CONTENT
                weight = 0f
            })
            val buttons = row(Look.SPACE_SM).apply { gravity = Gravity.END or Gravity.CENTER_VERTICAL }
            for ((label, action) in actions) {
                val (kind, run) = action
                buttons.addView(button(if (kind == ButtonKind.DANGER) ButtonKind.TEXT else kind, label).apply {
                    if (kind == ButtonKind.DANGER) {
                        setTextColor(palette.error)
                    }
                    setOnClickListener {
                        dialog.dismiss()
                        run()
                    }
                })
            }
            addView(buttons, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT))
        }
        dialog.setContentView(body)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.window?.setLayout(
            (context.resources.displayMetrics.widthPixels * DIALOG_WIDTH).toInt().coerceAtMost(px(DIALOG_MAX)),
            ViewGroup.LayoutParams.WRAP_CONTENT,
        )
        dialog.show()
    }

    private fun faceOf(kind: ButtonKind): Drawable = when (kind) {
        ButtonKind.FILLED -> rounded(palette.primary, Look.RADIUS_FULL)
        ButtonKind.TONAL -> rounded(palette.secondaryContainer, Look.RADIUS_FULL)
        ButtonKind.TEXT -> ColorDrawable(Color.TRANSPARENT)
        ButtonKind.DANGER -> rounded(palette.errorContainer, Look.RADIUS_FULL)
    }

    private fun inkOf(kind: ButtonKind): Int = when (kind) {
        ButtonKind.FILLED -> palette.onPrimary
        ButtonKind.TONAL -> palette.onSecondaryContainer
        ButtonKind.TEXT -> palette.primary
        ButtonKind.DANGER -> palette.onErrorContainer
    }

    private companion object {
        const val SEGMENT_INNER = 8f
        const val DIALOG_WIDTH = 0.86f
        const val DIALOG_MAX = 560f
    }
}
