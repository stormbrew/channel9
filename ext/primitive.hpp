#pragma once

namespace Channel9
{
	inline Value number_channel_simple(Environment *cenv, const Value &oself, const Message &msg)
	{
		if (msg.args().size() == 1 && msg.args()[0].m_type == MACHINE_NUM && msg.name().length() == 1)
		{
			long long other = msg.args()[0].machine_num, self = oself.machine_num;
			switch (msg.name()[0])
			{
			case '+':
				return value(self + other);
			case '-':
				return value(self - other);
			case '*':
				return value(self * other);
			case '/':
				return value(self / other);
			case '%':
				return value(self % other);
			}
		}
		printf("Unknown message for number %s\n", msg.name().c_str());
		exit(1);
	}
}