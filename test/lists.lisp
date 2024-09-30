;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - List creation: `list', `append', `cons'
;;   - Accessing lists: `car', `cdr'
;;   - List information: `length'
;;------------------------------------------------------------------------------

(list)          ; Expected: nil
(list 'a 'b 'c) ; Expected: (a b c)

(append)                   ; Expected: nil
(append nil)               ; Expected: nil
(append '(a) '(b c) '(d))  ; Expected: (a b c d)
(append '(a b) nil '(c d)) ; Expected: (a b c d)

(cons 'a nil)        ; Expected: (a)
(cons 'a '(b c))     ; Expected: (a b c)
(cons '(a b) '(y z)) ; Expected: ((a b) y z)

(car nil)    ; Expected: nil
(car '(a))   ; Expected: a
(car '(a b)) ; Expected: a

(cdr nil)      ; Expected: nil
(cdr '(a))     ; Expected: nil
(cdr '(a b c)) ; Expected: (b c)

(length  nil)     ; Expected: 0
(length '(a b c)) ; Expected: 3
