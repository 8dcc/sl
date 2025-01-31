;;------------------------------------------------------------------------------
;; Miscellaneous tests that don't fit in the other files.
;;------------------------------------------------------------------------------

unbound-error-var  ; Expected: Unbound symbol: unbound-error-var

(define var1 10)         ; Expected: 10
(define var2 (* 2 var1)) ; Expected: 20

(define my-addition +)  ; Expected: <primitive ...>
(my-addition var1 var2) ; Expected: 30

;; Because there is an implicit evaluation of each argument before calling
;; `apply', `eval' receives a symbol of content "my-addition". After evaluating
;; this simbol, the addition C primitive is returned, and then applied to `var1'
;; and `var2', which were also evaluated implicitly.
(define my-symbol 'my-addition) ; Expected: my-addition
((eval my-symbol) var1 var2)    ; Expected: 30

(equal? '(1 2 3) '(1 2 3))       ; Expected: tru
(equal? '(1 2 3) '(1 2 9))       ; Expected: nil

(equal? 1 1.0) ; Expected: nil
(= 1 1.0)      ; Expected: tru
(= 1 1.000001) ; Expected: nil

;; Symbol `nil' evaluates to itself, and an empty list is converted to `nil' in
;; the parser. See the manual for more information.
(type-of nil)     ; Expected: Symbol
(type-of 'nil)    ; Expected: Symbol
(type-of ())      ; Expected: Symbol
(type-of '())     ; Expected: Symbol
(equal? nil 'nil) ; Expected: tru

(type-of '(a b c)) ; Expected: Pair
(list? '(a b c))   ; Expected: tru
(list? '(a b . c)) ; Expected: nil

(> 1 1) ; Expected: nil
(> 1 2) ; Expected: nil
(> 2 1) ; Expected: tru

(< 1 1) ; Expected: nil
(< 2 1) ; Expected: nil
(< 1 2) ; Expected: tru

(< 1.0 2) ; Expected: tru
(> 2.0 1) ; Expected: tru

(or)                       ; Expected: nil
(and)                      ; Expected: tru
(or nil nil nil)           ; Expected: nil
(and 1 2 3)                ; Expected: 3
(or nil tru 'unreachable)  ; Expected: tru
(and 123 nil 'unreachable) ; Expected: nil

(set-random-seed 50)
(random 1337)
(random 10.0)
