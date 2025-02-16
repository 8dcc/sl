;; For more information on the Y combinator, see my blog article:
;; https://8dcc.github.io/programming/understanding-y-combinator.html

(define Y
  (lambda (f)
    ((lambda (x) (f (lambda (n) ((x x) n))))
     (lambda (x) (f (lambda (n) ((x x) n)))))))

(define fact-generator
  (lambda (self)
    (lambda (n)
      (if (equal? n 0)
          1
          (* n (self (- n 1)))))))

(define fact
  (Y fact-generator))

(fact 5)
