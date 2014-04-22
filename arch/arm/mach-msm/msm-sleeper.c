/*
 * ElementalX msm-sleeper by flar2 <asegaert@gmail.com>
 * 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <mach/cpufreq.h>
#include <linux/lcd_notify.h>

#define MSM_SLEEPER_MAJOR_VERSION	3
#define MSM_SLEEPER_MINOR_VERSION	0

extern uint32_t maxscroff;
extern uint32_t maxscroff_freq;

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu,
						  u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
    u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

    if (idle_time == -1ULL)
	return get_cpu_idle_time_jiffy(cpu, wall);
    else
	idle_time += get_cpu_iowait_time_us(cpu, wall);

    return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
    u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

    if (iowait_time == -1ULL)
	return 0;

    return iowait_time;
}


struct notifier_block notif;

static void msm_sleeper_suspend(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, maxscroff_freq);
		pr_info("Limit max frequency to: %d\n", maxscroff_freq);
	}

	return; 
}

static void msm_sleeper_resume(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, MSM_CPUFREQ_NO_LIMIT);
		pr_info("Restore max frequency to %d\n", MSM_CPUFREQ_NO_LIMIT);
	}

	return; 
}

static int lcd_notifier_callback(struct notifier_block *this,
				unsigned long event, void *data)
{
	switch (event) {
	case LCD_EVENT_ON_START:
		msm_sleeper_resume();
		break;
	case LCD_EVENT_ON_END:
		break;
	case LCD_EVENT_OFF_START:
		msm_sleeper_suspend();
		break;
	case LCD_EVENT_OFF_END:
		break;
	default:
		break;
	}

	return 0;
}


static int __init msm_sleeper_init(void)
{
	pr_info("msm-sleeper version %d.%d\n",
		 MSM_SLEEPER_MAJOR_VERSION,
		 MSM_SLEEPER_MINOR_VERSION);

	notif.notifier_call = lcd_notifier_callback;

	if (lcd_register_client(&notif))
		printk("[msm-sleeper] error\n");

	return 0;
}

MODULE_AUTHOR("flar2 <asegaert@gmail.com>");
MODULE_DESCRIPTION("'msm-sleeper' - Limit max frequency while screen is off");
MODULE_LICENSE("GPL v2");

late_initcall(msm_sleeper_init);

