# BGMN Parameter Reference

## Overview

BGMN refinement parameters are stored in `.par` files with the format:
```
PARAMETER_NAME = value [estimated_standard_deviation]
```

## Global Parameters

| Parameter | Description | Typical Initial Value |
|-----------|-------------|----------------------|
| `WMIN` | Minimum 2-theta angle (degrees) | 10.0 |
| `WMAX` | Maximum 2-theta angle (degrees) | 100.0 |
| `STEP` | Step size (degrees) | 0.02 |
| `LAMBDA` | X-ray wavelength | CU, MO, or custom value |
| `POL` | Polarization correction | `sqr(cos(26.6*pi/180))` for graphite mono |
| `ZEROSHIFT` | Zero shift correction (degrees 2-theta) | 0.0 |
| `DISPLACEMENT` | Sample displacement | 0.0 |
| `BACKGROUND` | Background polynomial | Chebyshev coefficients |

## Profile Parameters (Caglioti)

The Caglioti function describes peak width vs angle:
```
FWHM^2 = U * tan^2(theta) + V * tan(theta) + W
```

| Parameter | Description | Typical Value |
|-----------|-------------|---------------|
| `U` | Gaussian FWHM parameter | 0.01 |
| `V` | Gaussian FWHM parameter | -0.01 |
| `W` | Gaussian FWHM parameter | 0.005 |
| `NA` | Lorentzian mixing (Nelson-A) | 0.5 |
| `NB` | Lorentzian mixing (Nelson-B) | 0.0 |
| `X` | Lorentzian Scherrer broadening | 0.0 |
| `Y` | Lorentzian strain broadening | 0.0 |
| `ASYM` | Peak asymmetry | 0.0 |

## Phase Parameters

Each phase is indexed with brackets: `PARAM[1]`, `PARAM[2]`, etc.

| Parameter | Description | Typical Value |
|-----------|-------------|---------------|
| `SCALE[N]` | Phase scale factor | 1.0 |
| `WF[N]` | Weight fraction (%) | 100.0 |
| `A[N]` | Lattice parameter a (Angstrom) | varies |
| `B[N]` | Lattice parameter b (Angstrom) | varies |
| `C[N]` | Lattice parameter c (Angstrom) | varies |
| `ALPHA[N]` | Unit cell angle alpha (degrees) | 90.0 |
| `BETA[N]` | Unit cell angle beta (degrees) | 90.0 |
| `GAMMA[N]` | Unit cell angle gamma (degrees) | 90.0 |
| `VOL[N]` | Unit cell volume (Angstrom^3) | derived |
| `GRAIN_SIZE[N]` | Crystallite size (nm) | 100.0 |
| `MICROSTRAIN[N]` | Microstrain (epsilon0) | 0.0 |
| `R_BRAGG[N]` | Bragg R-factor | N/A (output) |

## R-Factors (output only)

| Parameter | Description | Target Value |
|-----------|-------------|--------------|
| `Rwp` | Weighted profile R-factor | < 10% for good fit |
| `Rexp` | Expected R-factor | depends on counting stats |
| `GoF` | Goodness of Fit (Rwp/Rexp) | 1.0 - 1.5 |
| `DW` | Durbin-Watson statistic | > 1.5 |

## Common .sav Control File Structure

```bgmn
% SampleID: MySample
VERZERR=instrument.geq          % Instrument geometry file
LAMBDA=CU                        % Wavelength
POL=sqr(cos(26.6*pi/180))        % Polarization
STRUC[1]=quartz.str              % Phase 1 structure
STRUC[2]=corundum.str            % Phase 2 structure
VAL[1]=data.xy                   % Measured data
WMIN=10                          % Angular range
WMAX=80
LIST=result.lst                  % Output listing
OUTPUT=result.par                % Output parameters
```

## Refinement Strategy Order

For best results, refine parameters in this order:
1. Background (scale + polynomial coefficients)
2. Zero shift
3. Lattice parameters (a, b, c)
4. Profile parameters (U, V, W)
5. Sample displacement
6. Phase scale factors
7. Atomic coordinates and thermal parameters
8. Preferred orientation
