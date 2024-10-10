;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - String evaluation (single-line and multi-line)
;;   - Common list primitives: `length', `append'
;;   - String creation: `write-to-str', `format', `substring'
;;   - String matching: `string-matches'
;;   - String predicates: `equal?', `<', `>'
;;------------------------------------------------------------------------------

"Hello, world!"

"Multi-line
strings
supported." ; Expected: "Multi-line\nstrings\nsupported."

(length "")    ; Expected: 0
(length "foo") ; Expected: 3

(append "" "")
(append "Concatenating" " multiple " "strings...")

(write-to-str '(+ 1 2 3 (- 5 4)))
(write-to-str (lambda (x)
                'symbol
                (func x 123 "Hello, world!\n")))

(format "%%s: %s %s %s" "Testing" "format" "specifiers!")
(format "%%d: %d %d %d" 1 2 3)
(format "%%f: %f %f %f" 1.0 2.0 3.0)

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

(equal?
 "All printed strings
must be valid inputs."                         ; Initial string
 "All printed strings\nmust be valid inputs.") ; Printed after evaluating

(< "abc" "abz") ; Expected: tru
(< "abc" "abc") ; Expected: nil
(> "abz" "abc") ; Expected: tru
(> "abc" "abc") ; Expected: nil
