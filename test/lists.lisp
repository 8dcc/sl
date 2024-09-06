;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Primitives for creating lists (`list', `append')
;;   - Primitives for accessing lists (`car', `cdr')
;;------------------------------------------------------------------------------

(list)          ; Expected: nil
(list 'a 'b 'c) ; Expected: (a b c)

(cons 'a nil)        ; Expected: (a)
(cons 'a '(b c))     ; Expected: (a b c)
(cons '(a b) '(y z)) ; Expected: ((a b) y z)

(car nil)    ; Expected: nil
(car '(a))   ; Expected: a
(car '(a b)) ; Expected: a

(cdr nil)      ; Expected: nil
(cdr '(a))     ; Expected: nil
(cdr '(a b c)) ; Expected: (b c)

(append)                   ; Expected: nil
(append nil)               ; Expected: nil
(append '(a) '(b c) '(d))  ; Expected: (a b c d)
(append '(a b) nil '(c d)) ; Expected: (a b c d)
