;;------------------------------------------------------------------------------
;; LoggedData monad primitives.

;; Extract the data from a LoggedData monad.
(defun LoggedData/get-data (logged-data)
  (car logged-data))

;; Wrap some data into an LoggedData monad.
(defun LoggedData/new (data logs)
  (cons data logs))

;; Bind some LoggedData to a transformation function. The transformation
;; function must receive one argument (the raw data that the binding function
;; will extract) and it should return a LoggedData.
;;
;; This function is essentially responsible for joining two log lists. The
;; 'LoggedData/get-logs' and 'LoggedData/concatenate-logs' functions have a
;; reduced scope to emphasize that they should only be used from within the
;; binding function.
(defun LoggedData/bind (logged-data transformation-function)
  (defun LoggedData/get-logs (logged-data)
    (cdr logged-data))
  (defun LoggedData/concatenate-logs (logged-data-1 logged-data-2)
    (append (LoggedData/get-logs logged-data-1)
            (LoggedData/get-logs logged-data-2)))
  (let ((transformation-result (transformation-function
                                 (LoggedData/get-data logged-data))))
    (LoggedData/new (LoggedData/get-data transformation-result)
                    (LoggedData/concatenate-logs logged-data
                                                 transformation-result))))

;;------------------------------------------------------------------------------
;; Helper functions.

;; Bind multiple transformation functions to an initial monadic input. This
;; could be abstracted even more by receiving the binding function as an
;; argument, and it would work for all monads.
(defun LoggedData/bind-chain (monadic-input &rest transformation-functions)
  (if (null? transformation-functions)
    monadic-input
    (reduce LoggedData/bind (cons monadic-input transformation-functions))))

;; Format a LoggedData monad into a string.
(defun LoggedData/format (logged-data)
  (format "Data: %d\nLogs:\n%s"
          (LoggedData/get-data logged-data)
          (apply append (mapcar (lambda (log-line)
                                  (append "  " log-line "\n"))
                                (cdr logged-data)))))

;;------------------------------------------------------------------------------
;; Transformation functions.

;; Add two integers toggether, logging the operation.
;;
;; Note how the function receives two plain integers, and returns a LoggedData
;; result.
(defun LoggedData/add-integers (a b)
  (assert (and (int? a) (int? b)))
  (let ((result (+ a b)))
    (LoggedData/new result
                    (list (format "Added %d+%d=%d" a b result)))))

;; Subtract two integers toggether, logging the operation.
(defun LoggedData/subtract-integers (a b)
  (assert (and (int? a) (int? b)))
  (let ((result (- a b)))
    (LoggedData/new result
                    (list (format "Subtracted %d-%d=%d" a b result)))))

;;------------------------------------------------------------------------------
;; Example.

(defun print-result (logged-data)
  (print-str (LoggedData/format logged-data))
  logged-data)

(print-result
  (LoggedData/bind
    (LoggedData/add-integers 5 6)
    (lambda (result1)
      (LoggedData/subtract-integers result1 3))))

(print-result
  (LoggedData/bind-chain
    (LoggedData/add-integers 5 6)
    (lambda (result1)
      (LoggedData/subtract-integers result1 3))
    (lambda (result2)
      (LoggedData/add-integers result2 4))))
