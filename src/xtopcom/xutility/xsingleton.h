// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
template<class T>
class xsingleton
{
public:
	static T* Instance()
	{
		if (NULL == m_pInstance)
		{
			if (NULL == m_pInstance)
			{
				m_pInstance = new T;
				atexit(Destory);
			}
		}

		return m_pInstance;
	}
	static void Destory()
	{
		if (m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
	}

protected:
	xsingleton() {} 
	xsingleton(const xsingleton&) {} 
	xsingleton& operator=(const xsingleton&) {} 

	virtual ~xsingleton()
	{
	}

private:
	static T* m_pInstance;
};

template<class T> T* xsingleton<T>::m_pInstance = NULL;
