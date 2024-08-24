;; Features tested in this source:
;;   - Variable definitions (global and local)
;;   - Function calls (lambdas and primitives)
;;   - Nested functions
;;   - Conditionals (if)
;;   - Logical operators (equal?)

;;------------------------------------------------------------------------------

;; Defined in the global environment
(define my-global 10)
(define my-addition +)

;; The lambda has its own environment, but since we also look in parent
;; environments, we can evalutate the `test-var' and `my-addition' globals.
(define test-lambda
  (lambda (x y)
    (* my-global (my-addition x y))))

(test-lambda 3 4)  ; Expected: 70

;;------------------------------------------------------------------------------

;; Since `nested-env-test' has its own environment, `nested-lambda' is defined
;; in there, so the scope is not global.
(define test-nested
  (lambda (x)
    (define my-nested-lambda
      (lambda (x)
        (* 2 x)))
    (my-nested-lambda x)))

(test-nested 5)  ; Expected: 10

;;------------------------------------------------------------------------------

;; Example function with conditional for calculating the factorial of a number
(define fact
  (lambda (n)
    (if (equal? n 0)
      1
      (* n (fact (- n 1))))))

(fact 5)  ; Expected: 120
