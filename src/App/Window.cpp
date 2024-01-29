#include "Window.h"
#include "Camera.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>

#include <array>
#include <iostream>
#include <filesystem>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define JSON_NOEXCEPTION
#define TINYGLTF_NOEXCEPTION


void Window::setAllBuffers() {
  vbos.reserve(model.bufferViews.size());
  ebos.reserve(model.bufferViews.size());

  for (size_t i = 0; i < model.bufferViews.size(); ++i) {
    const tinygltf::BufferView &bufferView = model.bufferViews[i];
    std::cout << i << " " << bufferView.target<< std::endl;
    if (bufferView.target == 0) {  // TODO impl drawarrays
      
      buffer_to_vbo[i] = vbos.size();
      vbos.emplace_back(model.buffers[bufferView.buffer], bufferView);
      continue;
    }

    std::cout << "--> load buffer: " << i << std::endl;
    if (bufferView.target == 34962) { // ARRAY_BUFFER
      buffer_to_vbo[i] = vbos.size();
      vbos.emplace_back(model.buffers[bufferView.buffer], bufferView);
      continue;
    }

    if (bufferView.target == 34963) { // ELEMENT_ARRAY_BUFFER
      buffer_to_ebo[i] = ebos.size();
      ebos.emplace_back(model.buffers[bufferView.buffer], bufferView);
      continue;
    }
    assert(false && "unreachable");
  }
}

void Window::bindMesh(tinygltf::Mesh &mesh) {
  for (size_t i = 0; i < mesh.primitives.size(); ++i) {
    todraw.emplace_back();
    todraw.back().vao->bind();

    tinygltf::Primitive primitive = mesh.primitives[i];
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

    if (indexAccessor.bufferView >= 0) {
        assert(buffer_to_ebo.find(indexAccessor.bufferView) != buffer_to_ebo.end()); 
        ebos[buffer_to_ebo[indexAccessor.bufferView]].bind();
        todraw.back().ebo = true;
    }
    std::cout << "--> bind ebo: " << indexAccessor.bufferView << std::endl;

    for (auto &attrib : primitive.attributes) {

      tinygltf::Accessor accessor = model.accessors[attrib.second];
      int byteStride =
          accessor.ByteStride(model.bufferViews[accessor.bufferView]);
     
      std::cout << "assert:  " <<  accessor.bufferView<< std::endl;

      std::cout << "--> bind vbo: " << accessor.bufferView << std::endl;

      int size = 1;
      if (accessor.type != TINYGLTF_TYPE_SCALAR) {
        size = accessor.type;
      }

      int vaa = -1;
      if (attrib.first.compare("POSITION") == 0) vaa = 0;
      if (attrib.first.compare("NORMAL") == 0) vaa = 1;
      if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
      if (vaa > -1) {
        std::cout << "--> enableAttributeArray: " << vaa << std::endl;

        assert(buffer_to_vbo.find(accessor.bufferView) != buffer_to_vbo.end()); 
        vbos[buffer_to_vbo[accessor.bufferView]].bind();

        program_->enableAttributeArray(vaa);
	      program_->setAttributeBuffer(vaa, accessor.componentType, accessor.byteOffset, size, byteStride);

      } else
        std::cout << "vaa missing: " << attrib.first << std::endl;
    }

    if (model.textures.size() > 0) {
      // fixme: Use material's baseColor
      tinygltf::Texture &tex = model.textures[0];

      if (tex.source > -1) {
        tinygltf::Image &image = model.images[tex.source];

        GLenum format = GL_RGBA;

        if (image.component == 1) {
          format = GL_RED;
        } else if (image.component == 2) {
          format = GL_RG;
        } else if (image.component == 3) {
          format = GL_RGB;
        } else {
          // ???
        }

        GLenum type = GL_UNSIGNED_BYTE;
        if (image.bits == 8) {
          // ok
        } else if (image.bits == 16) {
          type = GL_UNSIGNED_SHORT;
        } else {
          // ???
        }
        
        QImage im(image.image.data(), image.width, image.height, QImage::Format::Format_RGB32);
        todraw.back().texture = std::make_unique<QOpenGLTexture>(std::move(im));
        // todraw[ind].texture = ;
        todraw.back().texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        todraw.back().texture->setWrapMode(QOpenGLTexture::WrapMode::Repeat);

        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
        //              format, type, &image.image.at(0));
      }
    }
    todraw.back().indexAccessorCount = indexAccessor.count;
    todraw.back().indexAccessorComponentType = indexAccessor.componentType;
    todraw.back().indexAccessorByteOffset = indexAccessor.byteOffset;
    todraw.back().primitiveMode = primitive.mode;
    todraw.back().vao->release();
  }

}

// bind models
void Window::bindModelNodes(tinygltf::Node &node) {
  if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
    std::cout << "--> build node: " << node.mesh << std::endl;
    bindMesh(model.meshes[node.mesh]);
  }

  for (size_t i = 0; i < node.children.size(); i++) {
    assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
    bindModelNodes(model.nodes[node.children[i]]);
  }
}

void Window::bindModel() {
  const tinygltf::Scene &scene = model.scenes[model.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
    bindModelNodes(model.nodes[scene.nodes[i]]); // mesh -> primitives: indexBuf, attrBuf1, attrBuf2, ... + text1(material) 
  }
}


bool Window::loadModel(const char *filename) {
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool res = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cout << "ERR: " << err << std::endl;
  }

  if (!res)
    std::cout << "Failed to load glTF: " << filename << std::endl;
  else
    std::cout << "Loaded glTF: " << filename << std::endl;

  return res;
}

Window::Window() noexcept
{
	const auto formatFPS = [](const auto value, const auto morphing) {
		return QString("FPS: %1, morphing: %2").arg(QString::number(value), QString::number(morphing));
	};

  morphingSlider.setRange(0, 100);
	morphingSlider.setValue(0);

	auto fps = new QLabel(formatFPS(0, 1), this);
	fps->setStyleSheet("QLabel { color : white; }");

	auto layout = new QVBoxLayout();
	layout->addWidget(fps, 1);
  layout->addWidget(&morphingSlider);

	setLayout(layout);

	timer_.start();

	connect(this, &Window::updateUI, [=] {
		fps->setText(formatFPS(ui_.fps, morphingSlider.value()));
	});
  connect(&morphingSlider, SIGNAL(valueChanged(int)), &morphingSlider, SLOT(setValue(int)));
    
}

Window::~Window()
{
	{
		// Free resources with context bounded.
		const auto guard = bindContext();
		program_.reset();
	}
}

void Window::onInit()
{
  // "../src/App/Models/aaa.glb"
  const std::filesystem::path path = std::filesystem::absolute("../src/App/Models/monk.glb").lexically_normal();
  // const std::filesystem::path path = std::filesystem::absolute("src/App/Models/monk.glb").lexically_normal();
	if (!loadModel(path.string().c_str())) return;
   
	// Configure shaders
	program_ = std::make_unique<QOpenGLShaderProgram>(this);
	program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/diffuse.vs");
	program_->addShaderFromSourceFile(QOpenGLShader::Fragment,
									  ":/Shaders/diffuse.fs");
	program_->link();

	// vao_.create();
	// vao_.bind();

  program_->bind();
	// Bind model
  setAllBuffers();
  bindModel();

	mUniform_ = program_->uniformLocation("model");
  vUniform_ = program_->uniformLocation("view");
  pUniform_ = program_->uniformLocation("projection");
  
  morphingUniform_ = program_->uniformLocation("morphing");
  lightColorUniform_ = program_->uniformLocation("lightColor");
  lightPosUniform_ = program_->uniformLocation("lightPos");
  viewPosUniform_ = program_->uniformLocation("viewPos");
	// Release all
	program_->release();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::onRender()
{
	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Calculate MVP matrix
	model_.setToIdentity();
  //
  view_.setToIdentity();
  view_.translate(0, -2, -7);

	//const auto mvp = projection_ * view_ * model_;

	// Bind VAO and shader program
	program_->bind();

	// Update uniform value
	program_->setUniformValue(mUniform_, model_);
  program_->setUniformValue(vUniform_, cam.getView());
  program_->setUniformValue(pUniform_, projection_);
  program_->setUniformValue(lightColorUniform_, QVector3D(1., 0.8, 1.));
  program_->setUniformValue(lightPosUniform_, QVector3D(5., 5., 5.));
  program_->setUniformValue(viewPosUniform_, cam.getPos());


  morphing_.setX((double)morphingSlider.value() / 100);
	program_->setUniformValue(morphingUniform_, morphing_);
	// Activate texture unit and bind texture

	// Draw
  // drawModel();
 for (auto& model : todraw) {
      model.draw();
  }
	// Release VAO and shader program
	
	program_->release();

	++frameCount_;

	// Request redraw if animated
	if (animated_)
	{
		update();
	}
}

void Window::onResize(const size_t width, const size_t height)
{
	// Configure viewport
	glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));

	// Configure matrix
	const auto aspect = static_cast<float>(width) / static_cast<float>(height);
	const auto zNear = 0.1f;
	const auto zFar = 100.0f;
	const auto fov = 60.0f;
	projection_.setToIdentity();
	projection_.perspective(fov, aspect, zNear, zFar);
}

Window::PerfomanceMetricsGuard::PerfomanceMetricsGuard(std::function<void()> callback)
	: callback_{ std::move(callback) }
{
}

Window::PerfomanceMetricsGuard::~PerfomanceMetricsGuard()
{
	if (callback_)
	{
		callback_();
	}
}

auto Window::captureMetrics() -> PerfomanceMetricsGuard
{
	return PerfomanceMetricsGuard{
		[&] {
			if (timer_.elapsed() >= 1000)
			{
				const auto elapsedSeconds = static_cast<float>(timer_.restart()) / 1000.0f;
				ui_.fps = static_cast<size_t>(std::round(frameCount_ / elapsedSeconds));
				frameCount_ = 0;
				emit updateUI();
			}
		}
	};
}

void TODRAW::draw() {
  static QOpenGLFunctions funcs;
  static bool inited = false;
  if (!inited) {
     funcs.initializeOpenGLFunctions();
     inited = true;
  }
 
  vao->bind();
  funcs.glActiveTexture(GL_TEXTURE0);
  texture->bind();

  #define BUFFER_OFFSET(i) ((char *)NULL + (i))
  if (ebo) {
    funcs.glDrawElements(
      primitiveMode, 
      indexAccessorCount,
      indexAccessorComponentType,
      BUFFER_OFFSET(indexAccessorByteOffset));
  } else {
    funcs.glDrawArrays(
      primitiveMode,
      0,
      indexAccessorCount);
    }
  #undef BUFFER_OFFSET

  texture->release();
  vao->release();
}

void Window::keyReleaseEvent(QKeyEvent *event) {
   if(event->key() == Qt::Key_E)
    {
        cam.moveForward();
    }
    if(event->key() == Qt::Key_F)
    {
        cam.moveBackward();
    }
    if(event->key() == Qt::Key_W)
    {
        cam.moveUp();
    }
     if(event->key() == Qt::Key_S)
    {
        cam.moveDown();
    }
     if(event->key() == Qt::Key_A)
    {
        cam.moveLeft();
    }
     if(event->key() == Qt::Key_D)
    {
        cam.moveRight();
    }
}