#include "mainview.h"
#include "model.h"
#include "vertex.h"

#include <math.h>
#include <QDateTime>

/**
 * @brief MainView::MainView
 *
 * Constructor of MainView
 *
 * @param parent
 */
MainView::MainView(QWidget *parent) : QOpenGLWidget(parent) {
    qDebug() << "MainView constructor";

    connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
    viewTransform.translate(0, 0, -4);
}

/**
 * @brief MainView::~MainView
 *
 * Destructor of MainView
 * This is the last function called, before exit of the program
 * Use this to clean up your variables, buffers etc.
 *
 */
MainView::~MainView() {
    debugLogger->stopLogging();

    qDebug() << "MainView destructor";

    glDeleteTextures(numObjects, texturePtr);

    destroyModelBuffers();
}

// --- OpenGL initialization

/**
 * @brief MainView::initializeGL
 *
 * Called upon OpenGL initialization
 * Attaches a debugger and calls other init functions
 */
void MainView::initializeGL() {
    qDebug() << ":: Initializing OpenGL";
    initializeOpenGLFunctions();

    debugLogger = new QOpenGLDebugLogger();
    connect( debugLogger, SIGNAL( messageLogged( QOpenGLDebugMessage ) ),
             this, SLOT( onMessageLogged( QOpenGLDebugMessage ) ), Qt::DirectConnection );

    if ( debugLogger->initialize() ) {
        qDebug() << ":: Logging initialized";
        debugLogger->startLogging( QOpenGLDebugLogger::SynchronousLogging );
        debugLogger->enableMessages();
    }

    QString glVersion;
    glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qDebug() << ":: Using OpenGL" << qPrintable(glVersion);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0, 1.0, 0.0, 1.0);

    createShaderProgram();
    loadObjects();

    // Specify object movement
    object[0].rotationSpeed = 1.5;
    object[1].rotationSpeed = 1.0;
    object[2].rotationSpeed = 1.0;
    object[3].rotationSpeed = 1.3;
    object[0].scale = 1.2;
    object[1].scale = 0.5;
    object[2].scale = 1.1;
    object[3].scale = 0.9;

    // Initialize transformations
    updateProjectionTransform();


    timer.start(1000.0/60.0);
}

void MainView::loadObjects ()
{
    numObjects = 4;
    object = new Object[numObjects];
    meshVBO = new GLuint[numObjects];
    meshVAO = new GLuint[numObjects];
    texturePtr = new GLuint[numObjects];
    glGenBuffers(numObjects, meshVBO);
    glGenVertexArrays(numObjects, meshVAO);
    glGenTextures(numObjects, texturePtr);
    meshSize = new GLuint[numObjects];
    loadMesh (":/models/cat.obj", 1);
    loadMesh (":/models/cat.obj", 0);
    loadMesh (":/models/sphere.obj", 2);
    loadMesh (":/models/sphere.obj", 3);
    loadTexture (":/textures/cat_diff.png", texturePtr[0]);
    loadTexture (":/textures/cat_spec.png", texturePtr[1]);
    loadTexture (":/textures/wood1.jpg", texturePtr[2]);
    loadTexture (":/textures/wood2.jpg", texturePtr[3]);

    updateModelTransforms();
}

void MainView::createShaderProgram()
{
    // Create Normal Shader program
    normalShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                           ":/shaders/vertshader_normal.glsl");
    normalShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                           ":/shaders/fragshader_normal.glsl");
    normalShaderProgram.link();

    // Create Gouraud Shader program
    gouraudShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                           ":/shaders/vertshader_gouraud.glsl");
    gouraudShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                           ":/shaders/fragshader_gouraud.glsl");
    gouraudShaderProgram.link();

    // Create Phong Shader program
    phongShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                           ":/shaders/vertshader_phong.glsl");
    phongShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                           ":/shaders/fragshader_phong.glsl");
    phongShaderProgram.link();

    // Get the uniforms for the normal shader.
    uniformModelViewTransformNormal  = normalShaderProgram.uniformLocation("modelViewTransform");
    uniformProjectionTransformNormal = normalShaderProgram.uniformLocation("projectionTransform");
    uniformNormalTransformNormal     = normalShaderProgram.uniformLocation("normalTransform");

    // Get the uniforms for the gouraud shader.
    uniformModelViewTransformGouraud  = gouraudShaderProgram.uniformLocation("modelViewTransform");
    uniformProjectionTransformGouraud = gouraudShaderProgram.uniformLocation("projectionTransform");
    uniformNormalTransformGouraud     = gouraudShaderProgram.uniformLocation("normalTransform");
    uniformMaterialGouraud            = gouraudShaderProgram.uniformLocation("material");
    uniformLightPositionGouraud       = gouraudShaderProgram.uniformLocation("lightPosition");
    uniformLightColourGouraud         = gouraudShaderProgram.uniformLocation("lightColour");
    uniformTextureSamplerGouraud      = gouraudShaderProgram.uniformLocation("textureSampler");

    // Get the uniforms for the phong shader.
    uniformModelViewTransformPhong  = phongShaderProgram.uniformLocation("modelViewTransform");
    uniformProjectionTransformPhong = phongShaderProgram.uniformLocation("projectionTransform");
    uniformNormalTransformPhong     = phongShaderProgram.uniformLocation("normalTransform");
    uniformMaterialPhong            = phongShaderProgram.uniformLocation("material");
    uniformLightPositionPhong       = phongShaderProgram.uniformLocation("lightPosition");
    uniformLightColourPhong         = phongShaderProgram.uniformLocation("lightColour");
    uniformTextureSamplerPhong      = phongShaderProgram.uniformLocation("textureSampler");
}

void MainView::loadMesh(const char *path, GLuint idx)
{
    Model model(path);
    model.unitize();
    QVector<float> meshData = model.getVNTInterleaved();
    qDebug() << "Arrived here";
    this->meshSize[idx] = model.getVertices().size();
    qDebug() << "Ander here";
    // Bind VAO
    glBindVertexArray(meshVAO[idx]);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO[idx]);

    // Write the data to the buffer
    glBufferData(GL_ARRAY_BUFFER, meshData.size() * sizeof(float), meshData.data(), GL_STATIC_DRAW);

    // Set vertex coordinates to location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    // Set vertex normals to location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Set vertex texture coordinates to location 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void MainView::loadTexture(QString file, GLuint texturePtr)
{
    // Set texture parameters.
    glBindTexture(GL_TEXTURE_2D, texturePtr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Push image data to texture.
    QImage image(file);
    QVector<quint8> imageData = imageToBytes(image);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
}

// --- OpenGL drawing

/**
 * @brief MainView::paintGL
 *
 * Actual function used for drawing to the screen
 *
 */
void MainView::paintGL() {
    // Clear the screen before rendering
    glClearColor(0.2f, 0.5f, 0.7f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateModelTransforms();
    updateViewTransform();

    // Choose the selected shader.
    QOpenGLShaderProgram *shaderProgram;
    switch (currentShader) {
    case NORMAL:
        shaderProgram = &normalShaderProgram;
        shaderProgram->bind();
        break;
    case GOURAUD:
        shaderProgram = &gouraudShaderProgram;
        shaderProgram->bind();
        break;
    case PHONG:
        shaderProgram = &phongShaderProgram;
        shaderProgram->bind();
        break;
    }

    // Set all textures and draw the meshes.
    for (GLuint idx = 0; idx < numObjects; ++idx)
    {
        switch (currentShader) {
        case NORMAL: updateNormalUniforms(idx); break;
        case GOURAUD: updateGouraudUniforms(idx); break;
        case PHONG: updatePhongUniforms(idx); break;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturePtr[idx]);

        glBindVertexArray(meshVAO[idx]);
        glDrawArrays(GL_TRIANGLES, 0, meshSize[idx]);
    }
    shaderProgram->release();
}

/**
 * @brief MainView::resizeGL
 *
 * Called upon resizing of the screen
 *
 * @param newWidth
 * @param newHeight
 */
void MainView::resizeGL(int newWidth, int newHeight)
{
    Q_UNUSED(newWidth)
    Q_UNUSED(newHeight)
    updateProjectionTransform();
}

void MainView::updateNormalUniforms(GLuint idx)
{
    auto modelViewMatrix = viewTransform * object[idx].meshTransform;

    glUniformMatrix4fv(uniformProjectionTransformNormal, 1, GL_FALSE, projectionTransform.data());
    glUniformMatrix4fv(uniformModelViewTransformNormal, 1, GL_FALSE, modelViewMatrix.data());
    glUniformMatrix3fv(uniformNormalTransformNormal, 1, GL_FALSE, modelViewMatrix.normalMatrix().data());
}

void MainView::updateGouraudUniforms(GLuint idx)
{
    auto modelViewMatrix = viewTransform * object[idx].meshTransform;

    glUniformMatrix4fv(uniformProjectionTransformGouraud, 1, GL_FALSE, projectionTransform.data());
    glUniformMatrix4fv(uniformModelViewTransformGouraud, 1, GL_FALSE,  modelViewMatrix.data());
    glUniformMatrix3fv(uniformNormalTransformGouraud, 1, GL_FALSE, modelViewMatrix.normalMatrix().data());

    glUniform4fv(uniformMaterialGouraud, 1, &material[0]);
    glUniform3fv(uniformLightPositionGouraud, 1, &lightPosition[0]);
    glUniform3fv(uniformLightColourGouraud, 1, &lightColour[0]);

    glUniform1i(uniformTextureSamplerGouraud, 0); // Redundant now, but useful when you have multiple textures.
}

void MainView::updatePhongUniforms(GLuint idx)
{
    auto modelViewMatrix = viewTransform * object[idx].meshTransform;

    glUniformMatrix4fv(uniformProjectionTransformPhong, 1, GL_FALSE, projectionTransform.data());
    glUniformMatrix4fv(uniformModelViewTransformPhong, 1, GL_FALSE,  modelViewMatrix.data());
    glUniformMatrix3fv(uniformNormalTransformPhong, 1, GL_FALSE, modelViewMatrix.normalMatrix().data());

    glUniform4fv(uniformMaterialPhong, 1, &material[0]);
    glUniform3fv(uniformLightPositionPhong, 1, &lightPosition[0]);
    glUniform3fv(uniformLightColourPhong, 1, &lightColour[0]);

    glUniform1i(uniformTextureSamplerGouraud, 0);
}

void MainView::updateProjectionTransform()
{
    float aspect_ratio = static_cast<float>(width()) / static_cast<float>(height());
    projectionTransform.setToIdentity();
    projectionTransform.perspective(60, aspect_ratio, 0.2, 20);
    projectionTransform.rotate(QQuaternion::fromEulerAngles(viewRotation));
}

void MainView::updateModelTransforms()
{
    for (GLuint idx = 0 ; idx < numObjects ; ++idx)
    {
        object[idx].rotation.setY(object[idx].rotation.y() + object[idx].rotationSpeed);

        object[idx].meshTransform.setToIdentity();
        object[idx].meshTransform.translate(idx*2, 0, 0);
        object[idx].meshTransform.scale(object[idx].scale);
        object[idx].meshTransform.rotate(QQuaternion::fromEulerAngles(object[idx].rotation));
        object[idx].meshNormalTransform = object[idx].meshTransform.normalMatrix();
    }
    update();
}

void MainView::updateViewTransform()
{
    viewRotation.setY( viewRotation.y() + 0.1);

    viewTransform.setToIdentity();
    viewTransform.translate(0, 0, zoom);
    viewTransform.rotate(QQuaternion::fromEulerAngles(viewRotation));
}

// --- OpenGL cleanup helpers

void MainView::destroyModelBuffers()
{
    glDeleteBuffers(numObjects, meshVBO);
    glDeleteVertexArrays(numObjects, meshVAO);
}

// --- Public interface

void MainView::setRotation(int rotateX, int rotateY, int rotateZ)
{
    for (GLuint idx = 0 ; idx < numObjects ; ++idx)
    {
        object[idx].rotation = { static_cast<float>(rotateX), static_cast<float>(rotateY), static_cast<float>(rotateZ) };
    }
    updateModelTransforms();
}

void MainView::setViewRotation(float rotateX, float rotateY, float rotateZ)
{
    viewRotation.setX( viewRotation.x() + rotateX);
    viewRotation.setY( viewRotation.y() + rotateY);
    viewRotation.setZ( viewRotation.z() + rotateZ);

    updateViewTransform();
}

void MainView::updateViewDistance(float dist)
{
    zoom += 0.001*dist;

    updateViewTransform();
}

void MainView::setScale(int newScale)
{
    scale = static_cast<float>(newScale) / 100.f;
    updateModelTransforms();
}

void MainView::setShadingMode(ShadingMode shading)
{
    qDebug() << "Changed shading to" << shading;
    currentShader = shading;
}

// --- Private helpers

/**
 * @brief MainView::onMessageLogged
 *
 * OpenGL logging function, do not change
 *
 * @param Message
 */
void MainView::onMessageLogged( QOpenGLDebugMessage Message ) {
    qDebug() << " → Log:" << Message;
}

