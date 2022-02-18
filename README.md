# cqt-analyzer
Spectral Analyzer audio plugin based on the [Constant-Q transform](https://en.wikipedia.org/wiki/Constant-Q_transform). 
Hence, the plugin offers logarithmic equally spaced resolution across all octaves. It features a resolution of 48 bins per octave (1/8th tone) and currently covers 10 octaves. 
The original intention for this plugin was to serve as visualization for my [CQT implementation](https://github.com/jmerkt/rt-cqt).

The framework was recently changed to JUCE. But the main branch still constains the deprecated IPlug2 based files and submodules.

# Build Instructions
```
git clone https://github.com/jmerkt/cqt-analyzer --recursive
cd cqt-analyzer/CqtAnalyzer
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

