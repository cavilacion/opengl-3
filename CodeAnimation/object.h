#ifndef OBJECT_H
#define OBJECT_H

struct Object {
    QVector3D rotation;
    QMatrix3x3 meshNormalTransform;
    QMatrix4x4 meshTransform;
    float rotationSpeed;
    float scale;
} ;
#endif // OBJECT_H
