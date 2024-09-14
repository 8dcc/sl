;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - String evaluation (single-line and multi-line)
;;   - String predicates: `equal?', `<', `>'
;;------------------------------------------------------------------------------

"Hello, world!"
""

"Multi-line
strings
supported." ; Expected: "Multi-line\nstrings\nsupported."

(concat "Concatenating" " multiple " "strings...")

(equal?
 "All printed strings
must be valid inputs."                         ; Initial string
 "All printed strings\nmust be valid inputs.") ; Printed after evaluating

(< "abc" "abz") ; Expected: tru
(< "abc" "abc") ; Expected: nil
(> "abz" "abc") ; Expected: tru
(> "abc" "abc") ; Expected: nil
