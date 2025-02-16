;;------------------------------------------------------------------------------
;; Features tested in this source:
;;   - Variable definitions (global and local)
;;   - Function calls (lambdas and primitives)
;;   - Nested functions
;;   - Functions as arguments
;;   - Conditionals (if)
;;   - Logical primitives (equal?, >)
;;------------------------------------------------------------------------------

;; Defined in the global environment
(define my-global 10)
(define my-addition +)

;; The lambda has its own environment, but since we also look in parent
;; environments, we can evalutate the `test-var' and `my-addition' globals.
(define test-lambda
  (lambda (x y)
    (* my-global (my-addition x y))))
(test-lambda 3 4)

;; Since `nested-env-test' has its own environment, `nested-lambda' is defined
;; in there, so the scope is not global.
(define test-nested
  (lambda (x)
    (define my-nested-lambda
      (lambda (x)
        (* 2 x)))
    (my-nested-lambda x)))
(test-nested 5)

;; Example for showing how closures work. The inner lambda can access 'a' (bound
;; whenever the outer lambda is called) beacuse the parent environment of the
;; inner lambda is set whenever it was declared, not when it is called.
(define get-inner
  (lambda (a)
    (lambda (b)
      (+ a b))))
(define inner (get-inner 10))
(inner 20)

;; Example for testing functions as arguments.
(define summation
  (lambda (i n f)
    (if (> i n)
      0
      (+ (f i)
         (summation (+ i 1) n f)))))
(summation 1 5 (lambda (x) (* x 2)))

;; Example function with optional arguments.
(define test-rest
  (lambda (a b &rest etc)
    (+ a b
       (apply + etc))))
(test-rest 1 2 10 20)

;; Example recursive function for calculating the factorial of a number.
(define fact-recur
  (lambda (n)
    (if (equal? n 0)
      1
      (* n (fact-recur (- n 1))))))
(fact-recur 5)

;; Example iterative function for calculating the factorial of a number.
;; TODO: Tail-call optimization
(define fact-iter
  (lambda (n)
    (define iter
      (lambda (total i)
        (if (> i n)
          total
          (iter (* total i) (+ i 1)))))
    (iter 1 2)))
(fact-iter 5)
