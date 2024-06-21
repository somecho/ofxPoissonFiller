#pragma once

#include "of3dPrimitives.h"
#include "ofFbo.h"
#include "ofGraphics.h"
#include "ofShader.h"

class PoissonFiller {

  int _width, _height, depth;
  ofShader padder, downscale, filter, upscale, normalize;
  static const int size = 5;
  std::vector<ofFbo> ins, outs;
  ofFbo output, shift;
  static constexpr float h1[5] = {0.1507146, 0.6835785, 1.0334191, 0.6836,
                                  0.1507};
  static constexpr float g[3] = {0.0311849, 0.7752854, 0.0311849};
  ofPlanePrimitive quad;

  std::string shaderHeader = R"(
#version 330
bool isOutside(ivec2 pos, ivec2 size){
    return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}
in vec2 v_texCoord0;
out vec4 o_output;
uniform sampler2D tex0;
)";

  static std::string getVertexShader() {
    return R"(
#version 330
uniform mat4 modelViewProjectionMatrix;
in vec4 position;
in vec2 texcoord;
out vec2 v_texCoord0;
void main(){
	v_texCoord0 = texcoord;
	gl_Position = modelViewProjectionMatrix * position;
})";
  }

  std::string getPadderShader() {
    return shaderHeader + R"(
uniform int padding = 0;
void main(){
	ivec2 coords = ivec2(floor(gl_FragCoord.xy)-padding);
	if(isOutside(coords, textureSize(tex0, 0))){
		o_output = vec4(0.0);
		return;
	}
	o_output = texelFetch(tex0, coords,0);
})";
  }

  std::string getDownscaleShader() {
    return shaderHeader + R"(
uniform float h1[5];
uniform vec2 targetSize;
void main(){
    vec4 accum = vec4(0.0);
    ivec2 size = textureSize(tex0, 0).xy;
    ivec2 coords = ivec2(floor(targetSize * v_texCoord0)) * 2 - ivec2(10);
    for(int dy = -2; dy <=2; dy++){
		for(int dx = -2; dx <=2; dx++){
			ivec2 newPix = coords+ivec2(dx,dy);
			if(isOutside(newPix, size)){
				continue;
			}
			accum += h1[dx+2] * h1[dy+2] * texelFetch(tex0, newPix,0);
		}
	}
    o_output = accum;
})";
  }

  std::string getFilterShader() {
    return shaderHeader + R"(
uniform float g[3]; 
void main(){
    vec4 accum = vec4(0.0);
    ivec2 size = textureSize(tex0, 0).xy;
    ivec2 coords = ivec2(v_texCoord0 * vec2(size));
    for(int dy = -1; dy <=1; dy++){
        for(int dx = -1; dx <=1; dx++){
            ivec2 newPix = coords + ivec2(dx,dy);
            if(isOutside(newPix, size)){
                continue;
            }
            accum += g[dx+1] * g[dy+1] * texelFetch(tex0, newPix,0 );
        }
    }
    o_output = accum;
})";
  }

  std::string getUpscaleShader() {

    return shaderHeader + R"(
uniform sampler2D tex1; 
uniform float h1[5]; 
uniform float h2; 
uniform float g[3]; 

void main(){
    vec4 accum = vec4(0.0);
    ivec2 size = textureSize(tex0, 0).xy;
    ivec2 coords = ivec2(v_texCoord0 * vec2(size));
    for(int dy = -1; dy <=1; dy++){
        for(int dx = -1; dx <=1; dx++){
            ivec2 newPix = coords+ivec2(dx,dy);
            if(isOutside(newPix, size)){
                continue;
            }
            accum += g[dx+1] * g[dy+1] * texelFetch(tex0, newPix,0);
        }
    }
    ivec2 sizeSmall = textureSize(tex1, 0).xy;
    for(int dy = -2; dy <=2; dy++){
        for(int dx = -2; dx <=2; dx++){
            ivec2 newPix = coords+ivec2(dx,dy);
            newPix /= 2;
            newPix += 5;
            if(isOutside(newPix, sizeSmall)){
                continue;
            }
            accum += h2 * h1[dx+2] * h1[dy+2] * texelFetch(tex1, newPix, 0);
        }
    }
    o_output = accum;
})";
  }

  std::string getNormalizeShader() {
    return shaderHeader + R"(
void main(){
    vec4 fillColor = textureLod(tex0, v_texCoord0, 0.0);
    o_output.rgb = fillColor.rgb / fillColor.a;
    o_output.a = 1.0;
})";
  }

  void setupShader(ofShader &shader, std::string shaderSrc) {
    shader.setupShaderFromSource(GL_VERTEX_SHADER, getVertexShader());
    shader.setupShaderFromSource(GL_FRAGMENT_SHADER, shaderSrc);
    shader.bindDefaults();
    shader.linkProgram();
  }

  void drawQuad(float width, float height) {
    quad.set(width, height, 2, 2);
    quad.setPosition(width * 0.5, height * 0.5, 0.0);
    quad.draw();
  }

public:
  void init(int width, int height) {
    _width = width;
    _height = height;
    setupShader(padder, getPadderShader());
    setupShader(downscale, getDownscaleShader());
    setupShader(filter, getFilterShader());
    setupShader(upscale, getUpscaleShader());
    setupShader(normalize, getNormalizeShader());
    quad.mapTexCoords(0, 0, 1, 1);

    ofFboSettings settings;
    settings.internalformat = GL_RGBA32F;
    settings.width = _width;
    settings.height = _height;
    settings.textureTarget = GL_TEXTURE_2D;
    output.allocate(settings);
    shift.allocate(settings);

    settings.width = _width + 2 * size;
    settings.height = _height + 2 * size;
    depth = int(ceil(log2(std::min(_width, _height))));
    for (int i = 0; i < depth; i++) {
      ofFbo in, out;
      in.allocate(settings);
      out.allocate(settings);
      ins.push_back(in);
      outs.push_back(out);
      settings.width /= 2;
      settings.height /= 2;
      settings.width += 2 * size;
      settings.height += 2 * size;
    }
  }

  void process(ofFbo src) {
    ofDisableAlphaBlending();

    ins[0].begin();
    ofClear(0, 0, 0, 0);
    padder.begin();
    padder.setUniformTexture("tex0", src, 0);
    padder.setUniform1i("padding", size);
    drawQuad(ins[0].getWidth(), ins[0].getHeight());
    padder.end();
    ins[0].end();

    for (int i = 1; i < ins.size(); i++) {
      ins[i].begin();
      ofClear(0, 0, 0, 0);
      downscale.begin();
      downscale.setUniformTexture("tex0", ins[i - 1], 0);
      downscale.setUniform1fv("h1", h1, 5);
      downscale.setUniform2f("targetSize", ins[i].getWidth(),
                             ins[i].getHeight());
      drawQuad(ins[i].getWidth(), ins[i].getHeight());
      downscale.end();
      ins[i].end();
    }

    outs.back().begin();
    ofClear(0, 0, 0, 0);
    filter.begin();
    filter.setUniformTexture("tex0", ins.back(), 0);
    filter.setUniform1fv("g", g, 3);
    drawQuad(outs.back().getWidth(), outs.back().getHeight());
    filter.end();
    outs.back().end();

    for (int i = outs.size() - 2; i >= 0; i--) {
      outs[i].begin();
      ofClear(0, 0, 0, 0);
      upscale.begin();
      upscale.setUniform1fv("h1", h1, 5);
      upscale.setUniform1fv("g", g, 3);
      upscale.setUniform1f("h2", 0.0269546f);
      upscale.setUniformTexture("tex0", ins[i], 0);
      upscale.setUniformTexture("tex1", outs[i + 1], 1);
      drawQuad(outs[i].getWidth(), outs[i].getHeight());
      upscale.end();
      outs[i].end();
    }

    shift.begin();
    ofClear(0, 0, 0, 0);
    padder.begin();
    padder.setUniformTexture("tex0", outs.front(), 0);
    padder.setUniform1i("padding", -size);
    drawQuad(shift.getWidth(), shift.getHeight());
    padder.end();
    shift.end();

    output.begin();
    normalize.begin();
    normalize.setUniformTexture("tex0", shift, 0);
    drawQuad(ofGetWidth(), ofGetHeight());
    normalize.end();
    output.end();

    ofEnableAlphaBlending();
  }

  ofFbo &getResult() { return output; }
};
;