# Attenuator STLs

The attenuator STL files in this directory are based on the GPStar Technologies attenuator shell files.

## Original source

GPStar Technologies — GPStar-3D-Supports:

https://github.com/GPStarTechnologies/GPStar-3D-Supports/tree/d5fa58db6daba0661d32c0cbf3c415a4cb7ea438/stl/Attenuator

The upstream attenuator STL files are released under CC0 1.0.

## Repaired files

The following files were repaired for this project while preserving the original intended geometry and fit:

- `Attenuator - Radiation Lens Mount - Repaired.stl`
  - Removed disconnected/stray mesh fragments.
  - Consolidated extremely close duplicate vertices.
  - Result validated as a single-component, watertight, manifold mesh.

- `Attenuator - Radiation Lens Diffuser (Print Clear) - Repaired.stl`
  - Removed degenerate mesh artifacts.
  - Result validated as a single-component, watertight, manifold mesh.
  - Print in clear/transparent material as indicated by the upstream design.

Use the repaired versions above in preference to the corresponding original lens mount and diffuser STLs.
