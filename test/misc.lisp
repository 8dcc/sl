;;------------------------------------------------------------------------------
;; Miscellaneous tests that don't fit in the other files
;;------------------------------------------------------------------------------

unbound-error-var  ; Expected: Unbound symbol error

'(+ 1 2 3 (- 3 4) (* 3 4))
(+ 1 2 3 (- 3 4) (* 3 4)) ; Expected: 17

(list (mod  9.5  2.5)
      (mod  9.5 -2.5)
      (mod -9.5  2.5)
      (mod -9.5 -2.5)) ; Expected: (2.00 -0.50 0.50 -2.00)

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

(equal? '(1 2 3) '(1 2 3)) ; Expected: tru
(equal? '(1 2 3) '(1 2 9)) ; Expected: nil

(or)                       ; Expected: nil
(and)                      ; Expected: tru
(or nil nil nil)           ; Expected: nil
(and 1 2 3)                ; Expected: 3
(or nil tru 'unreachable)  ; Expected: tru
(and 123 nil 'unreachable) ; Expected: nil
