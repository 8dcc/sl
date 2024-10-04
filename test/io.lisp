;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Input primitives: `read', `read-str'
;;   - Output primitives: `write', `print-str'
;;------------------------------------------------------------------------------

(define writeln
    (lambda (e)
      (write e)
      (print-str "\n")
      tru))

(begin
 (print-str "Reading two expressions...\n")
 (writeln (read))
 (writeln (read)))

(begin
 (print-str "\nWriting two expressions...\n")
 (writeln '(+ 1 2 3 (f 'sym "str")))
 (writeln writeln))

(begin
 (print-str "\nReading two user-strings...\n")
 (writeln (read-str))
 (writeln (read-str ".\n")))
