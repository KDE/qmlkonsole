// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021-2022 Devin Lin <espidev@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.konsoleqml

Item {
    id: root

    required property TerminalEmulator terminalItem

    onEnabledChanged: {
        if (!enabled) {
            touchScrollHandler.stopInertia();
        }
    }

    TapHandler {
        acceptedDevices: PointerDevice.TouchScreen
        onPressedChanged: {
            if (pressed) {
                touchScrollHandler.pauseInertia();
            }
        }
        onTapped: touchScrollHandler.stopInertia()
    }

    DragHandler {
        id: touchScrollHandler

        acceptedDevices: PointerDevice.TouchScreen
        target: null
        xAxis.enabled: false

        property real inertialY
        property real previousInertialY
        property real wheelRemainder
        property real carriedVelocity
        property real initialInertialVelocity
        property double inertiaStartedAt

        readonly property real maximumVelocity: 10000
        readonly property real minimumVelocity: 50
        readonly property real deceleration: 2500

        function scrollBy(pixelDelta) {
            wheelRemainder += pixelDelta * 2;

            const wheelDelta = wheelRemainder < 0 ? Math.ceil(wheelRemainder) : Math.floor(wheelRemainder);
            if (wheelDelta === 0) {
                return;
            }

            root.terminalItem.simulateWheel(0, 0, 0, 0, Qt.point(0, wheelDelta));
            wheelRemainder -= wheelDelta;
        }

        function pauseInertia() {
            if (!touchScrollAnimation.running) {
                return;
            }

            const elapsed = (Date.now() - inertiaStartedAt) / 1000;
            carriedVelocity = Math.sign(initialInertialVelocity)
                * Math.max(0, Math.abs(initialInertialVelocity) - deceleration * elapsed);
            touchScrollAnimation.stop();
        }

        function stopInertia() {
            touchScrollAnimation.stop();
            carriedVelocity = 0;
            initialInertialVelocity = 0;
        }

        onActiveChanged: {
            if (active) {
                pauseInertia();
                return;
            }

            let velocity = centroid.velocity.y;
            if (Math.abs(velocity) < minimumVelocity) {
                stopInertia();
                return;
            }

            if (velocity * carriedVelocity > 0) {
                velocity += carriedVelocity;
            }
            velocity = Math.max(-maximumVelocity, Math.min(maximumVelocity, velocity));

            carriedVelocity = 0;
            initialInertialVelocity = velocity;
            inertiaStartedAt = Date.now();
            previousInertialY = 0;
            inertialY = 0;
            touchScrollAnimation.to = velocity * Math.abs(velocity) / (2 * deceleration);
            touchScrollAnimation.duration = Math.round(Math.abs(velocity) / deceleration * 1000);
            touchScrollAnimation.restart();
        }
        yAxis.onActiveValueChanged: delta => {
            if (active) {
                scrollBy(delta);
            }
        }
        onInertialYChanged: {
            scrollBy(inertialY - previousInertialY);
            previousInertialY = inertialY;
        }
    }

    NumberAnimation {
        id: touchScrollAnimation
        target: touchScrollHandler
        property: "inertialY"
        easing.type: Easing.OutQuad
        onFinished: {
            touchScrollHandler.carriedVelocity = 0;
            touchScrollHandler.initialInertialVelocity = 0;
        }
    }
}
