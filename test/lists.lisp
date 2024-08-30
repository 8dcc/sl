;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Primitives for creating lists (`list', `append')
;;   - Primitives for accessing lists (`car', `cdr')
;;------------------------------------------------------------------------------

(list)           ; Expected: nil
(list 'a 'b 'c)  ; Expected: (a b c)

(car nil)     ; Expected: nil
(car '(a))    ; Expected: a
(car '(a b))  ; Expected: a

(cdr nil)       ; Expected: nil
(cdr '(a))      ; Expected: nil
(cdr '(a b c))  ; Expected: (b c)

(append)                 ; Expected: nil
(append nil)             ; Expected: nil
(append 'a 'b 'c)        ; Expected: (a b c)
(append 'a nil 'b)       ; Expected: (a b)
(append 'a 'b '(1 2 3))  ; Expected: (a b 1 2 3)
