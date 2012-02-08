/*
 * Copyright (c) 2010
 *	Ben Gray <ben.r.gray@gmail.com>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ben Gray.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BEN GRAY ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BEN GRAY BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Power, Reset and Clock Managment Module
 *
 * This is a very simple driver wrapper around the PRCM set of registers in
 * the OMAP3 chip. It allows you to turn on and off things like the functional
 * and interface clocks to the various on-chip modules.
 *
 */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/resource.h>
#include <sys/rman.h>
#include <sys/lock.h>
#include <sys/mutex.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/frame.h>
#include <machine/resource.h>
#include <machine/intr.h>

#include <arm/ti/omap_prcm.h>

/**
 *	omap_clk_devmap - Array of clock devices, should be defined one per SoC 
 *
 *	This array is typically defined in one of the targeted omap??_prcm_clk.c
 *	files and is specific to the given SoC platform.  Each entry in the array
 *	corresponds to an individual clock device.
 */
extern struct omap_clock_dev omap_clk_devmap[];

/**
 *	omap_prcm_clk_dev - returns a pointer to the clock device with given id
 *	@clk: the ID of the clock device to get
 *
 *	Simply iterates through the clk_devmap global array and returns a pointer
 *	to the clock device if found. 
 *
 *	LOCKING:
 *	None
 *
 *	RETURNS:
 *	The pointer to the clock device on success, on failure NULL is returned.
 */
static struct omap_clock_dev *
omap_prcm_clk_dev(clk_ident_t clk)
{
	struct omap_clock_dev *clk_dev;
	
	/* Find the clock within the devmap - it's a bit inefficent having a for 
	 * loop for this, but this function should only called when a driver is 
	 * being activated so IMHO not a big issue.
	 */
	clk_dev = &(omap_clk_devmap[0]);
	while (clk_dev->id != INVALID_CLK_IDENT) {
		if (clk_dev->id == clk) {
			return (clk_dev);
		}
		clk_dev++;
	}

	/* Sanity check we managed to find the clock */
	printf("omap_prcm: Failed to find clock device (%d)\n", clk);
	return (NULL);
}

/**
 *	omap_prcm_clk_valid - enables a clock for a particular module
 *	@clk: identifier for the module to enable, see omap_prcm.h for a list
 *	      of possible modules.
 *	         Example: OMAP3_MODULE_MMC1_ICLK or OMAP3_MODULE_GPTIMER10_FCLK.
 *	
 *	This function can enable either a functional or interface clock.
 *
 *	The real work done to enable the clock is really done in the callback
 *	function associated with the clock, this function is simply a wrapper
 *	around that.
 *
 *	LOCKING:
 *	Internally locks the driver context.
 *
 *	RETURNS:
 *	Returns 0 on success or positive error code on failure.
 */
int
omap_prcm_clk_valid(clk_ident_t clk)
{
	int ret = 0;

	if (omap_prcm_clk_dev(clk) == NULL)
		ret = EINVAL;
	
	return (ret);
}


/**
 *	omap_prcm_clk_enable - enables a clock for a particular module
 *	@clk: identifier for the module to enable, see omap_prcm.h for a list
 *	      of possible modules.
 *	         Example: OMAP3_MODULE_MMC1_ICLK or OMAP3_MODULE_GPTIMER10_FCLK.
 *	
 *	This function can enable either a functional or interface clock.
 *
 *	The real work done to enable the clock is really done in the callback
 *	function associated with the clock, this function is simply a wrapper
 *	around that.
 *
 *	LOCKING:
 *	Internally locks the driver context.
 *
 *	RETURNS:
 *	Returns 0 on success or positive error code on failure.
 */
int
omap_prcm_clk_enable(clk_ident_t clk)
{
	struct omap_clock_dev *clk_dev;
	int ret;

	/* Find the clock within the devmap - it's a bit inefficent having a for 
	 * loop for this, but this function should only called when a driver is 
	 * being activated so IMHO not a big issue.
	 */
	clk_dev = omap_prcm_clk_dev(clk);

	/* Sanity check we managed to find the clock */
	if (clk_dev == NULL)
		return (EINVAL);

	/* Activate the clock */
	if (clk_dev->clk_activate)
		ret = clk_dev->clk_activate(clk_dev);
	else
		ret = EINVAL;

	return (ret);
}


/**
 *	omap_prcm_clk_disable - disables a clock for a particular module
 *	@clk: identifier for the module to enable, see omap_prcm.h for a list
 *	      of possible modules.
 *	         Example: OMAP3_MODULE_MMC1_ICLK or OMAP3_MODULE_GPTIMER10_FCLK.
 *	
 *	This function can enable either a functional or interface clock.
 *
 *	The real work done to enable the clock is really done in the callback
 *	function associated with the clock, this function is simply a wrapper
 *	around that.
 *
 *	LOCKING:
 *	Internally locks the driver context.
 *
 *	RETURNS:
 *	Returns 0 on success or positive error code on failure.
 */
int
omap_prcm_clk_disable(clk_ident_t clk)
{
	struct omap_clock_dev *clk_dev;
	int ret;

	/* Find the clock within the devmap - it's a bit inefficent having a for 
	 * loop for this, but this function should only called when a driver is 
	 * being activated so IMHO not a big issue.
	 */
	clk_dev = omap_prcm_clk_dev(clk);

	/* Sanity check we managed to find the clock */
	if (clk_dev == NULL)
		return (EINVAL);

	/* Activate the clock */
	if (clk_dev->clk_deactivate)
		ret = clk_dev->clk_deactivate(clk_dev);
	else
		ret = EINVAL;
	
	return (ret);
}

/**
 *	omap_prcm_clk_set_source - sets the source 
 *	@clk: identifier for the module to enable, see omap_prcm.h for a list
 *	      of possible modules.
 *	         Example: OMAP3_MODULE_MMC1_ICLK or OMAP3_MODULE_GPTIMER10_FCLK.
 *	
 *	This function can enable either a functional or interface clock.
 *
 *	The real work done to enable the clock is really done in the callback
 *	function associated with the clock, this function is simply a wrapper
 *	around that.
 *
 *	LOCKING:
 *	Internally locks the driver context.
 *
 *	RETURNS:
 *	Returns 0 on success or positive error code on failure.
 */
int
omap_prcm_clk_set_source(clk_ident_t clk, clk_src_t clksrc)
{
	struct omap_clock_dev *clk_dev;
	int ret;

	/* Find the clock within the devmap - it's a bit inefficent having a for 
	 * loop for this, but this function should only called when a driver is 
	 * being activated so IMHO not a big issue.
	 */
	clk_dev = omap_prcm_clk_dev(clk);

	/* Sanity check we managed to find the clock */
	if (clk_dev == NULL)
		return (EINVAL);

	/* Activate the clock */
	if (clk_dev->clk_set_source)
		ret = clk_dev->clk_set_source(clk_dev, clksrc);
	else
		ret = EINVAL;

	return (ret);
}


/**
 *	omap_prcm_clk_get_source_freq - gets the source clock frequency
 *	@clk: identifier for the module to enable, see omap_prcm.h for a list
 *	      of possible modules.
 *	@freq: pointer to an integer that upon return will contain the src freq
 *	
 *	This function returns the frequency of the source clock.
 *
 *	The real work done to enable the clock is really done in the callback
 *	function associated with the clock, this function is simply a wrapper
 *	around that.
 *
 *	LOCKING:
 *	Internally locks the driver context.
 *
 *	RETURNS:
 *	Returns 0 on success or positive error code on failure.
 */
int
omap_prcm_clk_get_source_freq(clk_ident_t clk, unsigned int *freq)
{
	struct omap_clock_dev *clk_dev;
	int ret;

	/* Find the clock within the devmap - it's a bit inefficent having a for 
	 * loop for this, but this function should only called when a driver is 
	 * being activated so IMHO not a big issue.
	 */
	clk_dev = omap_prcm_clk_dev(clk);

	/* Sanity check we managed to find the clock */
	if (clk_dev == NULL)
		return (EINVAL);

	/* Get the source frequency of the clock */
	if (clk_dev->clk_get_source_freq)
		ret = clk_dev->clk_get_source_freq(clk_dev, freq);
	else
		ret = EINVAL;
	
	return (ret);
}
