# Phase Identification Guidelines (Search-Match)

## Overview

Phase identification (Search-Match) is the process of identifying crystalline phases
present in an XRD pattern by comparing observed peak positions and intensities against
a database of known patterns.

## Databases

| Database | Content | Access |
|----------|---------|--------|
| **COD** (Crystallography Open Database) | 500,000+ entries, open access | Built into Profex |
| **ICDD PDF** (International Centre for Diffraction Data) | 400,000+ entries, commercial | Separate license needed |
| **ICSD** (Inorganic Crystal Structure Database) | 200,000+ structures | Separate license |

## Search-Match Workflow in Profex

### Step 1: Data Preparation
```
1. Import data file (drag & drop or File → Open)
2. Set proper wavelength (usually Cu Ka1 = 1.54056 Å)
3. Apply background subtraction if needed
4. Identify peak positions (Tools → Peak Search)
```

### Step 2: Search Parameters
```
1. Open Search-Match widget (View → Search-Match)
2. Select element filters (optional — narrows search)
3. Set search parameters:
   - d-spacing tolerance: 0.05-0.1 Å
   - Intensity cutoff: 1-5%
   - Figure of merit: FOM > 10 recommended
```

### Step 3: Candidate Evaluation
```
For each candidate phase, check:
✓ Major peaks match observed pattern
✓ All strong peaks accounted for
✓ Chemically reasonable (elements present)
✓ Consistent with sample history
```

### Step 4: Multi-Phase Analysis
```
When multiple phases are present:
1. Identify major phase first (strongest peaks)
2. Subtract or mark major phase peaks
3. Search remaining peaks for minor phases
4. Include all candidate phases in refinement
```

## COD Database in Profex

Profex includes built-in COD database support:

```bash
# COD database location (after installation)
~/.profex/cod/  # or %APPDATA%/Profex/cod/ on Windows

# Structure files in Profex format (.str)
profex/structures/*.str
```

### Installing COD:
1. Download from https://www.crystallography.net/
2. In Profex: Preferences → COD → Install/Update
3. Select COD mirror for download

## Key d-Spacing Reference

### Common Laboratory Phases

| Phase | Formula | Strongest d (Å) | 2-theta (Cu Ka) |
|-------|---------|-----------------|-------------------|
| Quartz | SiO₂ | 3.34 | 26.6° |
| Corundum | Al₂O₃ | 2.55 | 35.2° |
| Calcite | CaCO₃ | 3.04 | 29.4° |
| Halite | NaCl | 2.82 | 31.7° |
| Hematite | Fe₂O₃ | 2.70 | 33.2° |
| Magnetite | Fe₃O₄ | 2.53 | 35.4° |
| Rutile | TiO₂ | 3.25 | 27.4° |
| Anatase | TiO₂ | 3.52 | 25.3° |
| Kaolinite | Al₂Si₂O₅(OH)₄ | 7.16 | 12.4° |
| LaB₆ (NIST std) | LaB₆ | 4.16 | 21.4° |

## Automated Search-Match Tips

1. **Always verify with refinement**: Search-Match is a starting point.
   Always confirm by refining candidate phases.

2. **Check for preferred orientation**: Some phases (clays, layered materials)
   show anisotropic peak intensities due to preferred orientation.

3. **Look for amorphous content**: A broad hump at 15-35° 2-theta indicates
   amorphous glass or polymer content.

4. **Use chemical knowledge**: Don't waste time on phases containing elements
   you know are absent from your sample.

5. **Match d-spacings, not intensities**: Intensities are more affected by
   preferred orientation. Focus on d-spacing matches first.
