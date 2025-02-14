;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - List creation: `list', `append', `cons'
;;   - Accessing lists: `car', `cdr'
;;   - List information: `length'
;;------------------------------------------------------------------------------

(list)
(list 'a 'b 'c)

(append)
(append nil)
(append '(a) '(b c) '(d))
(append '(a b) nil '(c d))

(cons 'a nil)
(cons 'a 'b)
(cons 'a '(b . (c . nil)))
(cons 'a '(b c))
(cons '(a b) '(y z))

(car nil)
(car '(a))
(car '(a . b))
(car '(a b))

(cdr nil)
(cdr '(a))
(cdr '(a . b))
(cdr '(a b c))

(nth 2 '(a b c))

(length  nil)
(length '(a b c))
