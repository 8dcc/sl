;;------------------------------------------------------------------------------
;; Miscellaneous tests that don't fit in the other files.
;;------------------------------------------------------------------------------

unbound-error-var ; Intentional error.

(define var1 10)
(define var2 (* 2 var1))

(define my-addition +)
(my-addition var1 var2)

;; Because there is an implicit evaluation of each argument before calling
;; `apply', `eval' receives a symbol of content "my-addition". After evaluating
;; this simbol, the addition C primitive is returned, and then applied to `var1'
;; and `var2', which were also evaluated implicitly.
(define my-symbol 'my-addition)
((eval my-symbol) var1 var2)

(equal? '(1 2 3) '(1 2 3))
(equal? '(1 2 3) '(1 2 9))

(equal? 1 1.0)
(= 1 1.0)
(= 1 1.000001)

;; Symbol `nil' evaluates to itself, and an empty list is converted to `nil' in
;; the parser. See the manual for more information.
(type-of nil)
(type-of 'nil)
(type-of ())
(type-of '())
(equal? nil 'nil)

(type-of '(a b c))
(list? '(a b c))
(list? '(a b . c))

(> 1 1)
(> 1 2)
(> 2 1)

(< 1 1)
(< 2 1)
(< 1 2)

(< 1.0 2)
(> 2.0 1)

(or)
(and)
(or nil nil nil)
(and 1 2 3)
(or nil tru 'unreachable)
(and 123 nil 'unreachable)

(set-random-seed 50)
(random 1337)
(random 10.0)
