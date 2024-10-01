;;==============================================================================
;; Standard Lisp library for SL.
;; https://github.com/8dcc/sl
;;
;; TODO:
;;   - `let*'
;;   - `nth'
;;   - `member' (return arg0 when car is arg1)
;;   - `member?' (is arg1 in arg0?)
;;   - `string-split' using regex
;;==============================================================================

;;------------------------------------------------------------------------------
;; Meta-definition macros
;;------------------------------------------------------------------------------

;; (defmacro 1+ (var)  >  (define 1+
;;   (list '+ 1 var))  >    (macro (var)
;;                     >      (list '+ 1 var)))
(define defmacro
  (macro (name &rest macro-args)
    (list 'define name
          (cons 'macro macro-args))))

;; (defun my-function (n)  >  (define my-function
;;   (+ 1 n))              >    (lambda (n)
;;                         >      (+ 1 n)))
(defmacro defun (name &rest lambda-args)
  (list 'define name
        (cons 'lambda lambda-args)))

;;------------------------------------------------------------------------------
;; List-accessing functions
;;------------------------------------------------------------------------------

(defun caar (lst) (car (car lst)))
(defun cadr (lst) (car (cdr lst)))
(defun cdar (lst) (cdr (car lst)))
(defun cddr (lst) (cdr (cdr lst)))
(defun caaar (lst) (car (car (car lst))))
(defun caadr (lst) (car (car (cdr lst))))
(defun cadar (lst) (car (cdr (car lst))))
(defun caddr (lst) (car (cdr (cdr lst))))
(defun cdaar (lst) (cdr (car (car lst))))
(defun cdadr (lst) (cdr (car (cdr lst))))
(defun cddar (lst) (cdr (cdr (car lst))))
(defun cdddr (lst) (cdr (cdr (cdr lst))))

;;------------------------------------------------------------------------------
;; List-building functions
;;------------------------------------------------------------------------------

(defun cons* (&rest args)
  (if (null? (cdr args))
      (car args)
      (cons (car args)
            (apply cons* (cdr args)))))

;;------------------------------------------------------------------------------
;; Conditional macros
;;------------------------------------------------------------------------------

;; TODO: Support multiple expressions in clause without using begin?
;;
;; (cond (pred1 expr1)           >  (if pred1 (begin expr1)
;;       (pred2 expr2)           >    (if pred2 (begin expr2)
;;       (pred3 expr3a expr3b))  >      (if pred3 (begin expr3a expr3b)
;;                               >        nil)))
(defmacro cond (&rest clauses)
  (if (null? clauses)
      nil
      (list 'if (caar clauses)
            (cons 'begin (cdar clauses))
            (cons 'cond (cdr clauses)))))

;;------------------------------------------------------------------------------
;; Local variables
;;------------------------------------------------------------------------------

;; (let ((s1 e1)   >  ((lambda (s1 s2 s3)
;;       (s2 e2)   >     body1
;;       (s3 e3))  >     body2
;;   body1         >     body3)
;;   body2         >   e1 e2 e3)
;;   body3)        >
(defmacro let (definitions &rest body)
  (cons (cons 'lambda
              (cons (mapcar car definitions)
                    body))
        (mapcar cadr definitions)))

;;------------------------------------------------------------------------------
;; Debugging
;;------------------------------------------------------------------------------

;; TODO: Toggle by removing `func' if it's already in `*debug-trace*'
(defun trace (func)
  (define-global *debug-trace* (cons func *debug-trace*))
  "Trace enabled.")

;;------------------------------------------------------------------------------
;; General predicates
;;------------------------------------------------------------------------------

(defun not (predicate)
  (if predicate nil tru))

(defun null? (expr)
  (not expr))

(defun every (f lst)
  (cond ((null? lst) tru)
        ((not (f (car lst))) nil)
        (tru (every f (cdr lst)))))

(defun some (f lst)
  (if (null? lst)
      nil
      (let ((result (f (car lst))))
        (if result result
            (some f (cdr lst))))))

;; NOTE: Should match C's `expr_is_number'
(defun number? (expr)
  (or (int? expr)
      (flt? expr)))

;; NOTE: Should match C's `expr_is_applicable'
(defun applicable? (expr)
  (or (primitive? expr)
      (lambda?    expr)
      (macro?     expr)))

(defun = (&rest nums)
  (and (every number? nums)
       (apply equal? nums)))

;;------------------------------------------------------------------------------
;; Mapping
;;------------------------------------------------------------------------------

(defun mapcar (f lst)
  (if (null? lst)
      nil
      (cons (f (car lst))
            (mapcar f (cdr lst)))))
