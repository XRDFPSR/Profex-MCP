# Rietveld Refinement Guidelines

## Introduction

Rietveld refinement is a whole-pattern fitting method for powder diffraction data.
The goal is to minimize the weighted difference between observed and calculated
patterns by adjusting structural and instrumental parameters.

## Understanding R-Factors

| R-Factor | Formula | Good | Acceptable | Poor |
|----------|---------|------|------------|------|
| Rwp | sqrt(Σw(yᵢ-ycᵢ)² / Σw(yᵢ)²) | < 10% | 10-20% | > 20% |
| Rexp | depends on counting statistics | — | — | — |
| GoF | Rwp/Rexp | 1.0-1.5 | 1.5-2.0 | > 2.0 |
| DW | Durbin-Watson | > 1.5 | 1.0-1.5 | < 1.0 |

## Common Problems and Solutions

### 1. High Background
**Symptoms**: Rwp > 20%, poor fit at low angles
**Causes**: 
- Insufficient background polynomial order (try 6-12 terms)
- Amorphous content not modeled
- Fluorescence from sample (change wavelength)
**Solutions**: Increase background order, add amorphous phase, use fluorescence correction

### 2. Peak Shape Mismatch
**Symptoms**: Peaks too narrow/broad in model vs data
**Causes**:
- Incorrect instrument parameters (U, V, W)
- Crystallite size / microstrain not modeled
**Solutions**: 
- Refine U, V, W first
- Add GRAIN_SIZE and MICROSTRAIN parameters
- Check instrument geometry file (.geq)

### 3. Peak Position Offset
**Symptoms**: Systematic shift across all peaks
**Causes**:
- Zero shift error
- Sample displacement
**Solutions**:
- Refine ZEROSHIFT (allows uniform shift)
- Refine DISPLACEMENT (angle-dependent shift)
- Check sample height alignment

### 4. Preferred Orientation
**Symptoms**: Some peaks systematically too high/low
**Causes**: Non-random crystallite orientation (common in plate-like or needle-like crystals)
**Solutions**:
- Apply March-Dollase preferred orientation correction
- Use spherical harmonics for texture

### 5. Phase Quantification Issues
**Symptoms**: Phase fractions don't sum to 100%
**Causes**:
- Amorphous content present
- Extinction effects
- Microabsorption (different absorption coefficients)
**Solutions**:
- Add internal standard (e.g., 20% Si or Al₂O₃)
- Use Brindley correction for microabsorption

## Refinement Strategy (Recommended Sequence)

### Stage 1: Instrument Parameters
1. Background (scale + 6-8 coefficients)
2. Zero shift
3. Sample displacement
4. Profile parameters (U, V, W)

### Stage 2: Unit Cell Parameters
5. Lattice parameters for each phase
6. Phase scale factors

### Stage 3: Structural Parameters
7. Atomic coordinates (if needed)
8. Isotropic thermal parameters (Biso)
9. Site occupancies

### Stage 4: Microstructure
10. Crystallite size
11. Microstrain
12. Preferred orientation

## Quality Indicators

A good Rietveld refinement should show:
- **Visual**: Flat difference curve, no systematic features
- **Rwp**: < 10% for laboratory data
- **GoF**: 1.0 - 1.5
- **DW**: > 1.5
- **Cell parameters**: Agree with literature values within 0.1%
- **ESDs**: Reasonable (not artificially small)

## References

- Rietveld, H. M. (1969). J. Appl. Cryst., 2, 65-71.
- Young, R. A. (1993). The Rietveld Method, IUCr.
- McCusker, L. B. et al. (1999). J. Appl. Cryst., 32, 36-50.
