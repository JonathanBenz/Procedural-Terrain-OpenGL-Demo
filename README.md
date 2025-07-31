# Procedural Terrain OpenGL Demo - Acerola 2025 Dirtjam Submission
#### by Jonathan Benz

**Description:**  
My entry for the [Acerola 2025 Dirtjam](https://itch.io/jam/acerola-dirt-jam) — a procedural terrain demo built in Modern Core OpenGL using C++14 over the span of two weeks. 

---

## Features

- **Procedural Terrain Generation**
  - Heightmap generation done with random [Simplex Noise](https://github.com/SRombauts/SimplexNoise) and Fractional Brownian Motion.
  - Normal map generation done by calculating the gradients of each point of the heightmap.

- **Advanced Shading**
  - [Parallax Occlusion Mapping](https://learnopengl.com/Advanced-Lighting/Parallax-Mapping) on terrain surface to give the illusion of depth. 
  - [Blinn-Phong](https://learnopengl.com/Advanced-Lighting/Advanced-Lighting) Lighting Model.
  - Height-based snow texturing for mountain peaks.
  - Fog effect when camera is far away from the scene. 
  - Skybox [cubemap](https://learnopengl.com/Advanced-OpenGL/Cubemaps) environment.

- **Post-Processing**
  - [HDR and Tonemapping](https://learnopengl.com/Advanced-Lighting/HDR). 
  - Two-Pass Gaussian Blur for [Bloom](https://learnopengl.com/Advanced-Lighting/Bloom), using a downsampled framebuffer for increased performance. 
  - [Screen Space Lens Flare](https://john-chapman.github.io/2017/11/05/pseudo-lens-flare.html). 

---

## GIFs and Images
<p align="center">
  <img src="images/gifs/procTerrainDemoScene.gif" alt="Procedural Terrain Demo Scene"/>
  <p align="center"><em>Figure 1: Finalized Demo Scene.</em></p>
</p>

<p align="center">
  <img src="images/gifs/lensFlareDemo3Shorter.gif" alt="Lens Flare Demo 1" style="width:100%;margin-right:5%;"/>
  <img src="images/gifs/lensFlareDemo.gif" alt="Lens Flare Demo 2" style="width:100%;"/>
  <p align="center"><em>Figure 2: Screen Space Lens Flare Post-Process Effect.</em></p>
</p>

<p align="center">
  <img src="images/screenshots/demoSceneSimplexNoise.png" alt="Simplex Noise Visualization" height = "200" style="width:33%;margin-right:5%;"/>
  <img src="images/screenshots/demoSceneHeightmapNoFBM.png" alt="Heightmap Without Fractional Brownian Motion" style="width:33%;margin-right:5%;"/>
  <img src="images/screenshots/demoSceneHeightmapWithFBM.png" alt="Heightmap With Fractional Brownian Motion" style="width:33%"/>
  <p align="center"><em>Figure 3: From left to right: Simplex Noise Visualization, Heightmap Parallax Mapped without Fractional Brownian Motion (FBM), and with FBM. With FBM, the terrain looks more rocky and mountainous when octaves are layered upon it.</em></p>
</p>

<p align="center">
  <img src="images/screenshots/demoSceneProcTerrainNoSnow.png" alt="Textured Terrain" style="width:49%;margin-right:5%;"/>
  <img src="images/screenshots/demoSceneProcTerrainWithSnow.png" alt="Textured Terrain with Snowy Peaks" style="width:49%;"/>
  <p align="center"><em>Figure 4: From left to right: Terrain with base texture, and terrain with added snow texture dependent on the value of the heightmap (in TBN space) in order to achieve snowy mountain peaks. </em></p>
</p>

<p align="center">
  <img src="images/screenshots/demoSceneNoGammaCorrection.png" alt="No Gamma Correction" style="width:49%;margin-right:5%;"/>
  <img src="images/screenshots/demoSceneWithGammaCorrection.png" alt="Gamma Correction" height = "260" style="width:49%;"/>
  <p align="center"><em>Figure 5: From left to right: Rendering SRGB textures without <a href="https://learnopengl.com/Advanced-Lighting/Gamma-Correction" target="_blank">gamma correction</a>, versus with gamma correction. </em></p>
</p>

<p align="center">
  <img src="images/screenshots/lensFlareNoSunburst.png" alt="Lens Flare Without Sunburst" style="width:49%;margin-right:5%;"/>
  <img src="images/screenshots/LensFlareWithSunburst.png" alt="Lens Flare With Sunburst"  height = "255" style="width:49%;"/>
  <p align="center"><em>Figure 6: From left to right: Lens Flare effect without textured sunburst, versus with a textured sunburst. </em></p>
</p>

<p align="center">
  <img src="images/screenshots/blinnPhongDemo.png" alt="Blinn-Phong Specular Showcase"/>
  <p align="center"><em>Figure 7: Blinn-Phong exagerrated specular shininess showcase, with bloom. </em></p>
</p>

---

## Potential Future Work
- **[Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping) Implementation**
  - This gets a little tricky with Parallax Mapping, since every point on the quad shares the same z-value.

- **Volumetric Clouds**
  - This is something I have never done before, but it would be interesting to learn about. 
 
