/*
 * singleton.h
 *
 *  Created on: 2016. 1. 26.
 *      Author: shin
 */

#ifndef SRC_LIB_BASE_SRC_SINGLETON_H_
#define SRC_LIB_BASE_SRC_SINGLETON_H_

template <typename T>
class singleton_T
{
protected:
    singleton_T() {}
    virtual ~singleton_T() {
    		DestroyInstance();
    }

public:
    static T *GetInstance()
    {
        if (m_pInstance == nullptr)
            m_pInstance = new T;
        return m_pInstance;
    };

    static void DestroyInstance()
    {
        if (m_pInstance)
        {
            delete m_pInstance;
            m_pInstance = nullptr;
        }
    };

private:
    static T *m_pInstance;
};

template <typename T>
T *singleton_T<T>::m_pInstance = nullptr;

#endif /* SRC_LIB_BASE_SRC_SINGLETON_H_ */
