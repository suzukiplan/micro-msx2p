package com.suzukiplan.msx2_android

data class InputStatus(
    var up: Boolean,
    var down: Boolean,
    var left: Boolean,
    var right: Boolean,
    var a: Boolean,
    var b: Boolean,
    var select: Boolean,
    var start: Boolean
) {
    val code: Int
        get() {
            var value = 0
            if (up) value = value.or(0b00000001)
            if (down) value = value.or(0b00000010)
            if (left) value = value.or(0b00000100)
            if (right) value = value.or(0b00001000)
            if (a) value = value.or(0b00010000)
            if (b) value = value.or(0b00100000)
            if (start) value = value.or(0b01000000)
            if (select) value = value.or(0b10000000)
            return value
        }
}
