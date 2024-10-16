;; Print a list of expressions, along with their evaluations, in a format
;; suitable for the SL manual.
;;
;; FIXME: Use proper cons alists with format (EXPR . RESULT), not (EXPR RESULT)
;; FIXME: The scope of the evaluated expressions does not represent the normal
;;        evaluation in the REPL. Affects environment-sensitive functions like
;;        `define'.

(defun print-expr (expr evaluated)
  (write expr)
  (print-str "\n  ⇒ ")
  (write evaluated)
  (print-str "\n\n"))

(defun print-expr-alist (alist)
  (if (null? alist)
      'done
      (begin
       (print-expr (caar alist) (cadar alist))
       (print-expr-alist (cdr alist)))))

(defmacro print-exprs (&rest exprs)
  `(print-expr-alist
    (quote ,(mapcar (lambda (e)
                      (list e (eval e)))
                    exprs))))

;;------------------------------------------------------------------------------

(print-exprs
 ;; NOTE: Add the expressions here.
 (substring "abcdef")
 (substring "abcdef" 0 2)
 (substring "abcdef" 1 nil)
 (substring "abcdef" -1 nil)
 (substring "abcdef" 1 -1)
 (substring "abcdef" -3 -1)
 )

;; If you prefer a REPL-like interface:
;;
;;     (defun main-loop ()
;;       (let ((e (read)))
;;         (print-expr e (eval e)))
;;       (main-loop))
;;     (main-loop)
