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

(defmacro test-regexp (regexp ignore-case)
  `(string-matches ,regexp "Testing regular expressions... 123" ,ignore-case))

(test-regexp "Testing regular" nil)       ; Expected: ((0 15))
(test-regexp "testing REGULAR" tru)       ; Expected: ((0 15))
(test-regexp "^(Testing).*$" nil)         ; Expected: ((0 34) (0 7))
(test-regexp "^INVALID.*$" nil)           ; Expected: nil
(test-regexp "^(.+) ([[:digit:]]+)$" nil) ; Expected: ((0 34) (0 30) (31 34))

(equal?
 "All printed strings
must be valid inputs."                         ; Initial string
 "All printed strings\nmust be valid inputs.") ; Printed after evaluating

(< "abc" "abz") ; Expected: tru
(< "abc" "abc") ; Expected: nil
(> "abz" "abc") ; Expected: tru
(> "abc" "abc") ; Expected: nil
