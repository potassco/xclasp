# xclasp [![Deprecated](https://img.shields.io/badge/status-deprecated-yellow.svg)](https://github.com/potassco/clasp)

> A variant of the clasp solver for extracting learned constraints

## Overview

`xclasp` was an extension of the answer set solver [`clasp`](https://github.com/potassco/clasp) that allowed for logging the learned constraints.

The extracted constraints could then be reused by offline procedures.
For instance, [`ginkgo`](https://github.com/potassco/ginkgo/) generalizes learned constraints.

`xclasp` is now obsolete, and its functionality has fully been integrated into [`clasp`](https://github.com/potassco/clasp) with the 3.2 release.

## Replacing xclasp with clasp

To extract learned constraints, `xclasp` was usually invoked as follows:

```bash
xclasp --log-learnts --resolution-scheme=named \
       --heuristic=Domain --dom-mod=1,16 --loops=no --reverse-arcs=0 --otfs=0
```

The same result can now be achieved with `clasp`:

```bash
clasp --lemma-out=- --lemma-out-txt --lemma-out-dom=output \
      --heuristic=Domain --dom-mod=1,16 --loops=no --reverse-arcs=0 --otfs=0
```

The command-line option `--lemma-out=<file>` logs the extracted constraints to the specified file or to `stdout` if `-` is specified instead of a file.

`--lemma-out=txt` prints the constraints in text form instead of `aspif`.

`--lemma-out-dom=output` ensures that only such constraints are logged that contain only (named) output variables.

`clasp -h3` provides more details on the new options.

## Legacy Code

For reference, `xclasp`’s original code is archived in the [`xclasp`](https://github.com/potassco/xclasp/tree/xclasp) branch.

## Literature

* Martin Gebser, Roland Kaminski, Benjamin Kaufmann, [Patrick Lühne](https://www.luehne.de), Javier Romero, and Torsten Schaub: [*Answer Set Solving with Generalized Learned 
Constraints*](http://software.imdea.org/Conferences/ICLP2016/Proceedings/ICLP-TCs/p09-gebser.pdf). In: Technical Communications of the [32nd International Conference on Logic 
Programming](http://software.imdea.org/Conferences/ICLP2016/), 2016, pp. 9:1–9:14

* [Patrick Lühne](https://www.luehne.de): [*Generalizing Learned Knowledge in Answer Set 
Solving*](https://www.luehne.de/theses/generalizing-learned-knowledge-in-answer-set-solving.pdf). M.Sc. Thesis, 2015, Hasso Plattner Institute, Potsdam

## Contributors

* Benjamin Kaufmann (`xclasp`, integration into `clasp`)
* [Patrick Lühne](https://www.luehne.de) (`xclasp`)

