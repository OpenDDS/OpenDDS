// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_BASICQUEUELINK_T_H
#define TAO_DCPS_BASICQUEUELINK_T_H

namespace TAO
{

  namespace DCPS
  {

    template <typename T>
    class BasicQueueLink
    {
      public:

        BasicQueueLink()
          : elem_(0),
            next_(0)
          {
          }


        BasicQueueLink(T* value)
          : elem_(value),
            next_(0)
          {
          }


        /// Accessor for elem_ data memeber.
        T* elem()
          {
            return this->elem_;
          }

        /// Mutator for elem_ data memeber.
        void elem(T* value)
          {
            this->elem_ = value;
          }

        /// Accessor for reference to the elem_ data memeber.
        T*& elem_ref()
          {
            return this->elem_;
          }

        /// Accessor for next_ data member.
        BasicQueueLink<T>* next()
          {
            return this->next_;
          }

        /// Mutator for next_ data member.
        void next(BasicQueueLink<T>* value)
          {
            this->next_ = value;
          }

        /// Changes state of this object to match the state of the
        /// object following (default) construction.
        void reset()
          {
            this->elem_ = 0;
            this->next_ = 0;
          }


      private:

        /// The "payload" of the link -- the actual pointer that is
        /// being "wrapped" inside this link.
        T* elem_;

        /// The next link, or NULL (0) if this is the last link.
        BasicQueueLink<T>* next_;
    };

  }

}

#endif  /* TAO_DCPS_BASICQUEUELINK_T_H */
