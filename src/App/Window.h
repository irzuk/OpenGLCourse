#pragma once
#include "Camera.h"

#include <Base/GLWidget.hpp>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QSlider>

#include <functional>
#include <memory>
#include <iostream>

#include <tinygltf/tiny_gltf.h>

struct BufferVbo {
  BufferVbo(const tinygltf::Buffer& buf, const tinygltf::BufferView& bufView) {
     glBuffer_.create();
     bind();
     glBuffer_.setUsagePattern(QOpenGLBuffer::StaticDraw);
     glBuffer_.allocate(buf.data.data() + bufView.byteOffset, bufView.byteLength);
  }

  void bind() {
    glBuffer_.bind();
  }

//   ~BufferVbo() {
//     glBuffer_.release();
//   }
private:
  QOpenGLBuffer glBuffer_{QOpenGLBuffer::Type::VertexBuffer};

};

struct BufferEbo {
  BufferEbo(const tinygltf::Buffer& buf, const tinygltf::BufferView& bufView) {
     glBuffer_.create();
     bind();
     glBuffer_.setUsagePattern(QOpenGLBuffer::StaticDraw);
     glBuffer_.allocate(buf.data.data() + bufView.byteOffset, bufView.byteLength);
  }

  void bind() {
    glBuffer_.bind();
  }

//   ~BufferEbo() {
//     glBuffer_.release();
//   }
private:
  QOpenGLBuffer glBuffer_{QOpenGLBuffer::Type::IndexBuffer};

};

struct TODRAW {

	TODRAW() {
		vao->create();
	};

    std::unique_ptr<QOpenGLVertexArrayObject> vao = std::make_unique<QOpenGLVertexArrayObject>();
	std::unique_ptr<QOpenGLTexture> texture;
	bool ebo = false;

    int primitiveMode;
	size_t indexAccessorCount;
	int indexAccessorComponentType;
    size_t indexAccessorByteOffset;

	void draw();
};

class Window final : public fgl::GLWidget
{
	Q_OBJECT
public:
	Window() noexcept;
	~Window() override;

public: // fgl::GLWidget
	void onInit() override;
	void onRender() override;
	void onResize(size_t width, size_t height) override;
	//
	void keyReleaseEvent(QKeyEvent *event) override;

private:
	class PerfomanceMetricsGuard final
	{
	public:
		explicit PerfomanceMetricsGuard(std::function<void()> callback);
		~PerfomanceMetricsGuard();

		PerfomanceMetricsGuard(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard(PerfomanceMetricsGuard &&) = delete;

		PerfomanceMetricsGuard & operator=(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard & operator=(PerfomanceMetricsGuard &&) = delete;

	private:
		std::function<void()> callback_;
	};

private:
	[[nodiscard]] PerfomanceMetricsGuard captureMetrics();

signals:
	void updateUI();

private:
    my::Camera cam;
	GLint mUniform_ = -1;
	GLint vUniform_ = -1;
	GLint pUniform_ = -1;
	GLint lightColorUniform_ = -1;
	GLint lightPosUniform_ = -1;
	GLint viewPosUniform_ = -1;
	// GLint vUniform_ = -1;
	// GLint pUniform_ = -1;
	GLint morphingUniform_ = -1;
	QVector3D morphing_{1., 0., 0.}; 
	QSlider morphingSlider = QSlider(Qt::Horizontal);

    std::vector<TODRAW> todraw;

	QMatrix4x4 model_;
	QMatrix4x4 view_;
	QMatrix4x4 projection_;

	std::unique_ptr<QOpenGLShaderProgram> program_;
	

	QElapsedTimer timer_;
	size_t frameCount_ = 0;

	struct {
		size_t fps = 0;
	} ui_;

	bool animated_ = true;

	tinygltf::Model model;
	std::vector<BufferVbo> vbos;
    std::vector<BufferEbo> ebos;

	std::map<int, GLuint> buffer_to_vbo;
	std::map<int, GLuint> buffer_to_ebo;

    bool loadModel(const char *filename);
	void setAllBuffers();

	void bindMesh(tinygltf::Mesh &mesh);
	void bindModelNodes(tinygltf::Node &node);
	void bindModel();

	void drawModel();
	void drawModelNodes(tinygltf::Node &node);
	void drawMesh(tinygltf::Mesh &mesh);
};
