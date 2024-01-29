#pragma once

#include <QMatrix4x4>

#include <iostream>

namespace my {
struct Camera {

    QMatrix4x4 getView() {
        QMatrix4x4 view;
        view.lookAt(cameraPos, cameraTarget, cameraUp);
        return view;
    }

    QVector3D getPos() {
        return cameraPos;
    }

    void moveRight() {
        QMatrix4x4 m;
        auto proj = QVector3D(0., 1., 0.);
        m.rotate(cameraSpeed * 100, proj);
        cameraPos = m * cameraPos;
    }
    void moveLeft() {
        QMatrix4x4 m;
        auto proj = QVector3D(0., 1., 0.);
        m.rotate(-cameraSpeed * 100, proj);
        cameraPos = m * cameraPos;
    }
    void moveUp() {
        auto ax = QVector3D::crossProduct(QVector3D(0., 1., 0.), cameraPos);
        QMatrix4x4 m;
        m.rotate(-cameraSpeed * 100, ax);
        cameraPos = m * cameraPos;
    }
    void moveDown() {
        auto ax = QVector3D::crossProduct(QVector3D(0., 1., 0.), cameraPos);
        QMatrix4x4 m;
        m.rotate(cameraSpeed * 100, ax);
        cameraPos = m * cameraPos;
    }
    void moveForward() {
        cameraPos *= 1 - cameraSpeed;
    }
    void moveBackward() {
        cameraPos *= 1 + cameraSpeed;
    }

private:
   double cameraSpeed = 0.1;
   QVector3D cameraPos{3., 0., 0.};
   QVector3D cameraTarget = QVector3D(0.0f, 0.0f, 0.0f);
   QVector3D cameraUp = QVector3D(0.0f, 1.0f, 0.0f);
};
} // namespace my