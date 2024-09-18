;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - String evaluation (single-line and multi-line)
;;   - String predicates: `equal?', `<', `>'
;;   - String creation: `concat', `substring'
;;   - String matching: `string-matches'
;;------------------------------------------------------------------------------

"Hello, world!"
""

"Multi-line
strings
supported." ; Expected: "Multi-line\nstrings\nsupported."

(equal?
 "All printed strings
must be valid inputs."                         ; Initial string
 "All printed strings\nmust be valid inputs.") ; Printed after evaluating

(< "abc" "abz") ; Expected: tru
(< "abc" "abc") ; Expected: nil
(> "abz" "abc") ; Expected: tru
(> "abc" "abc") ; Expected: nil

(concat "Concatenating" " multiple " "strings...")

(substring "--Testing substrings--")
(substring "--Testing substrings--" 2 20)
(substring "--Testing substrings--" 2 -2)
(substring "--Testing substrings--" 10)
(substring "--Testing substrings--" -12)

(string-matches "Testing regular" "Testing regular expressions... 123")
(string-matches "testing regular" "Testing regular expressions... 123" tru)
(string-matches "^Testing\\.*$" "Testing regular expressions... 123")
(string-matches "^INVALID\\.*$" "Testing regular expressions... 123")
(string-matches "^(.+) ([[:digit:]]+)$" "Testing regular expressions... 123")
