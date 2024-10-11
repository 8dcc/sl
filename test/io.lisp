;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Input primitives: `read', `scan-str'
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
 (print-str "\nScanning two user strings...\n")
 (writeln (scan-str))
 (writeln (scan-str ".\n")))
