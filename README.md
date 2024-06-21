# ofxPoissonFiller


An OpenFrameworks addon for poisson filling based on convolution pyramids
([Farbman et al,
2011](https://pages.cs.huji.ac.il/danix-lab/cglab/projects/convpyr/)).

https://github.com/somecho/ofxPoissonFiller/assets/26333602/c45370d8-e3ae-411e-be8f-86a69301a673

This add on is header only, inspired by [ofxPoissonFill](https://github.com/LingDong-/ofxPoissonFill), but unlike ofxPoissonFill, uses GL_RGBA32F textures.

## Usage

```cpp
PoissonFiller pf;
ofFbo fbo;

// ofApp::setup
// ofxPoissonFiller only takes GL_RGBA32F textures
fbo.allocate(_WIDTH, _HEIGHT, GL_RGBA32F);
pf.init(fbo.getWidth(), fbo.getHeight())

// ofApp::update
pf.process(fbo);

// ofApp::draw
pf.getResult().draw(0,0);
```

## Reference
This addon is not a re-implementation of the convolution pyramid algorithm from scratch, but is an effort to port existing to work into OpenFrameworks. 

This addon is heavily based on:
- [kosua20/Rendu](https://github.com/kosua20/Rendu)
- [LingDong-/ofxPoissonFill](https://github.com/kosua20/Rendu)
- [orx-poisson-fill](https://github.com/openrndr/orx/tree/master/orx-jvm/orx-poisson-fill)
- [patriciogonzalezvivo/lygia](https://github.com/patriciogonzalezvivo/lygia_examples/blob/main/morphological_poissonFill.frag)

For further reading:
- [Convolution Pyramids, Farbman et al, 2021](https://pages.cs.huji.ac.il/danix-lab/cglab/projects/convpyr/)
- https://x.com/patriciogv/status/1402000238123044875