# PolyMesh_Benchmark

## Generel
- Momentan werden Benchmark-Ergebnisse in einer csv gespeichert um sie später mit py zu analysieren und zu plotten.
- Als Testmesh erstelle ich momentan simple eine UV Sphere und speicher sie als off Datei, dann kann jede Meshstruktur einfach die Datei lesen.
- Bisher werden alle Biblioteken (Eigen, OpenMesh) mit einer festen Version gepullt.


## Current PolyMesh Design Gedanken
- Für die Vertices eine Matrix(3,n) zu benutzen erlaubt matrix operationen, aber Matrix(n,3) ist für reine Iteration effizienter (linear effizienter im Cache).
- Da bool matricen nicht ohne typecasten nicht mit float matricen multipliziert werdne können, sollte / muss ich vielleicht zu float sparse matricen wechseln. 
- Ansonnsten würden folgende Matricen sehr viel Sinn machen (Edge x Vertex, Face x Vertex, Face x Edge), dafür müsste ich aber erstmal die jetzige Speichereffizienz mehr analysieren / mit anderen Strukturen vergleichen und dazu gibt es noch das nachfolgende Problem. 
- Die n x n Matricen wie die adjacency Matrix könnte ich auch also upper/lower Dreiecksmatrix speichern, damit würde sich der Specherverbrauch dieser Matricen halbieren.
- Ich könte auch die adjacency matrix weglassen und  eine Face x Vertex Matrix (fv) erstellen. Dann könnte man falls man die adjacency Matrix brauch, diese durch matrixmultiplikation errechnen ( fv^T * fv ).


### Speicher-Benchmark Probleme
- Speichereffizienz kann in cpp nicht so einfach ausgelesen werden. Die beiden Möglichkeiten die mir dazu nur einfallen sind entweder den Speicherverbrauch nur teoretisch durchzurechnen oder den verbrauchten Speicher so zu minimierern, dass wenn große Netze geladen werden, dass dier overhead des restlichen Programs mehr vernachlässigt werden kann und ich dann den gesammten Programspeicherverbrauch messe für zunehmende Netzgrößen.
