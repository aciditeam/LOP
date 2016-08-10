#include <Python.h>
#include <stdio.h>
#include "needleman.h"

#define U_FEPS 1.192e-6F          /* 1.0F + E_FEPS != 1.0F */
#define U_DEPS 2.22e-15           /* 1.0 +  E_DEPS != 1.0  */

#define E_FPEQ(a,b,e) (((b - e) < a) && (a < (b + e)))

#define _DIAG 	0
#define _UP  	1
#define _LEFT 	2

static float getScore(const int *ix, const int *iy, const int *m, int lena, int lenb,
					  int *start1, int *start2, int endweight)
{
    int i,j, cursor;
    float score = INT_MIN;
    *start1 = lena-1;
    *start2 = lenb-1;
    
    if(endweight)
    {
        /* when using end gap penalties the score of the optimal global
         * alignment is stored in the final cell of the path matrix */
		cursor = lena * lenb - 1;
		if(m[cursor]>ix[cursor]&&m[cursor]>iy[cursor])
			score = m[cursor];
		else if(ix[cursor]>iy[cursor])
			score = ix[cursor];
		else
			score = iy[cursor];
    }
    else {
		
        for (i = 0; i < lenb; ++i)
        {
            cursor = (lena - 1) * lenb + i;
            if(m[cursor]>score)
            {
				*start2 = i;
				score = m[cursor];
            }
            if(ix[cursor]>score)
            {
				score = ix[cursor];
				*start2 = i;
            }
            if(iy[cursor]>score)
            {
				score = iy[cursor];
				*start2 = i;
            }
        }
		
        for (j = 0; j < lena; ++j)
        {
            cursor = j * lenb + lenb - 1;
            if(m[cursor]>score)
            {
				*start1 = j;
				*start2 = lenb-1;
				score = m[cursor];
            }
            if(ix[cursor]>score)
            {
				score = ix[cursor];
				*start1 = j;
				*start2 = lenb-1;
            }
            if(iy[cursor]>score)
            {
				score = iy[cursor];
				*start1 = j;
				*start2 = lenb-1;
            }
        }
    }
    return score;
}

float _needleEmboss(const char *a, const char *b, int endweight, int gapopen, int gapextend,
		int endgapopen, int endgapextend)
{
    int		xpos, ypos;
    int		lena, lenb;
    int		bconvcode;
    int		match;
    int		ixp;
    int		iyp;
    int		mp;
	int		i, j;
    int		cursor, cursorp;
    int		*start1, *start2;
	int		nbAligns = 0;
    char	*recon1, *recon2;
    FILE	*outputFile;
    int		verbose = 0;
    int		testog;
    int		testeg;
    int		score = 0;
	/* Align stats : nbId, F, nbGaps, F, nbDiffs, F */
	int		stats[6] = {0, 0, 0, 0, 0, 0};

    if (endweight == 0)
    {
    	endgapopen = 0;
    	endgapextend = 0;
    	endweight = 1;
    }
	start1 = calloc(1, sizeof(int));
	start2 = calloc(1, sizeof(int));
    for (lena = 0; a[lena] != 0; lena++);
    for (lenb = 0; b[lenb] != 0; lenb++);
    match = DNAFull[a[0] - 'A'][b[0] - 'A'];
    int *compass = malloc(lena * lenb * sizeof(int));
    int *ix = malloc(lena * lenb * sizeof(int));
    int *iy = malloc(lena * lenb * sizeof(int));
    int *m = malloc(lena * lenb * sizeof(int));
    ix[0] = -endgapopen-gapopen;
    iy[0] = -endgapopen-gapopen;
    m[0] = match;
    cursor = 0;
    /* First initialise the first column */
    for (ypos = 1; ypos < lena; ++ypos)
    {
    	match = DNAFull[a[ypos] - 'A'][b[0] - 'A'];
		cursor = ypos * lenb;
    	cursorp = (ypos-1) * lenb;
    	testog = m[cursorp] - gapopen;
    	testeg = iy[cursorp] - gapextend;
    	if(testog >= testeg)
    		iy[cursor] = testog;
    	else
    		iy[cursor] = testeg;
    	m[cursor] = match - (endgapopen + (ypos - 1) * endgapextend);
    	ix[cursor] = -endgapopen - ypos * endgapextend - gapopen;
    }
    ix[cursor] -= endgapopen;
    ix[cursor] += gapopen;
    cursor=0;

    /* Now initialise the first row */
    for (xpos = 1; xpos < lenb; ++xpos)
    {
    	match = DNAFull[a[0] - 'A'][b[xpos] - 'A'];
		cursor = xpos;
    	cursorp = xpos -1;
    	testog = m[cursorp] - gapopen;
        testeg = ix[cursorp] - gapextend;
        if(testog >= testeg)
            ix[cursor] = testog;
        else
            ix[cursor] = testeg;
        m[cursor] = match - (endgapopen + (xpos - 1) * endgapextend);
        iy[cursor] = -endgapopen - xpos * endgapextend -gapopen;
    }
    iy[cursor] -= endgapopen;
    iy[cursor] += gapopen;
    xpos = 1;
	/* Now construct match, ix, and iy matrices */
    while (xpos != lenb)
    {
        ypos = 1;
        bconvcode = b[xpos] - 'A';

        /* coordinates of the cells being processed */
        cursorp = xpos-1;
        cursor = xpos++;

        while (ypos < lena)
        {
            /* get match for current xpos/ypos */
			match = DNAFull[a[ypos++] - 'A'][bconvcode];
			cursor += lenb;
            /* match matrix calculations */
            mp = m[cursorp];
            ixp = ix[cursorp];
            iyp = iy[cursorp];
            if(mp > ixp && mp > iyp)
                m[cursor] = mp+match;
            else if(ixp > iyp)
                m[cursor] = ixp+match;
            else
                m[cursor] = iyp+match;
            /* iy matrix calculations */
            if(xpos==lenb)
            {
            	testog = m[++cursorp] - endgapopen;
            	testeg = iy[cursorp] - endgapextend;
            }
            else
            {
            	testog = m[++cursorp];
            	if (testog<ix[cursorp])
            		testog = ix[cursorp];
            	testog -= gapopen;
            	testeg = iy[cursorp] - gapextend;
            }
            if(testog > testeg)
                iy[cursor] = testog;
            else
            	iy[cursor] = testeg;
            cursorp += lenb;
            /* ix matrix calculations */
            if(ypos==lena)
            {
            	testog = m[--cursorp] - endgapopen;
            	testeg = ix[cursorp] - endgapextend;
            }
            else
            {
            	testog = m[--cursorp];
            	if (testog<iy[cursorp])
            		testog = iy[cursorp];
            	testog -= gapopen;
            	testeg = ix[cursorp] - gapextend;
            }
            if (testog > testeg)
                ix[cursor] = testog;
            else
        	ix[cursor] = testeg;

        }
    }
	getScore(ix, iy, m, lena, lenb, start1, start2, endweight);
	xpos = *start2;
	ypos = *start1;
    cursorp=0;
    cursor= 1;
	/*
	 * Trace-back step
	 */
	while (xpos>=0 && ypos>=0)
    {
    	cursor = ypos*lenb+xpos;
    	mp = m[cursor];
    	if(cursorp == _LEFT && E_FPEQ((ypos==0||(ypos==lena-1)?
    			endgapextend:gapextend), (ix[cursor]-ix[cursor+1]),U_FEPS))
    	{
    		compass[cursor] = _LEFT;
    		xpos--;
    	}
    	else if(cursorp== _UP && E_FPEQ((xpos==0||(xpos==lenb-1)?
    			endgapextend:gapextend), (iy[cursor]-iy[cursor+lenb]),U_FEPS))
    	{
    		compass[cursor] = _UP;
    		ypos--;
    	}
    	else if(mp >= ix[cursor] && mp>= iy[cursor])
    	{
    		if(cursorp == _LEFT && E_FPEQ(mp,ix[cursor],U_FEPS))
    		{
        		compass[cursor] = _LEFT;
    			xpos--;
    		}
    		else if(cursorp == _UP && E_FPEQ(mp,iy[cursor],U_FEPS))
    		{
    			compass[cursor] = _UP;
    			ypos--;
    		}
    		else
    		{
    			compass[cursor] = 0;
    			ypos--;
    			xpos--;
    		}
		}
		else if(ix[cursor]>=iy[cursor] && xpos>-1)
		{
	    	compass[cursor] = _LEFT;
	    	xpos--;
		}
		else if(ypos>-1)
		{
			compass[cursor] = _UP;
	    	ypos--;
		}
		cursorp = compass[cursor];
    }
	xpos = *start2;
	ypos = *start1;
    if (verbose != 0)
    {
    	recon1 = malloc((lenb + lena) * sizeof(char));
    	recon2 = malloc((lenb + lena) * sizeof(char));
    }
    for (i = lenb - 1; i > xpos;)
    {
		stats[2]++;
		recon1[nbAligns] = '-';
		recon2[nbAligns++] = b[i--];
    }
    for (j = lena - 1; j > ypos;)
    {
		stats[2]++;
		recon1[nbAligns] = a[j--];
		recon2[nbAligns++] = '-';
    }
	while (xpos >= 0 && ypos >= 0)
    {
        cursor = ypos * lenb + xpos;
        switch (compass[cursor])
        {
			case _DIAG:
				stats[0] += (a[ypos] == b[xpos] ? 1 : 0);
				recon1[nbAligns] = a[ypos--];
				recon2[nbAligns++] = b[xpos--];
				break;
			case _LEFT:
				stats[2]++;
				recon1[nbAligns] = '-';
				recon2[nbAligns++] = b[xpos--];
				break;
			case _UP:
				stats[2]++;
				recon1[nbAligns] = a[ypos--];
				recon2[nbAligns++] = '-';
				break;
			default:
				break;
		}
    }
    for (; xpos >= 0 ; xpos--)
    {
		stats[2]++;
		recon1[nbAligns] = '-';
		recon2[nbAligns++] = b[xpos];
    }
    for (; ypos >= 0; ypos--)
    {
		stats[2]++;
		recon1[nbAligns] = a[ypos];
		recon2[nbAligns++] = '-';
    }
	if (verbose != 0)
        {
    		int i,j;
			outputFile = fopen("needleman_align_EMB.txt", "a+");
        	fprintf(outputFile, "**************************\n");
            fprintf(outputFile, "Length \t\t: %d\n", nbAligns);
            fprintf(outputFile, "Identities \t: %d (%.2f%%)\n", stats[0], 100.0f * (float)stats[0]/nbAligns);
			fprintf(outputFile, "Gaps \t\t: %d (%f%%)\n", stats[2], 100.0f * (float)stats[2]/nbAligns);
        	fprintf(outputFile, "**************************\n\n");
        	for (j = nbAligns - 1; j >= 0; j -= 50)
        	{
        		for (i = j; i > j - 50 && i >= 0; i--)
        			fprintf(outputFile, "%c", recon1[i]);
        		fprintf(outputFile, "\n");
        		for (i = j; i > j - 50 && i >= 0; i--)
        			if (recon1[i] == recon2[i])
        				fprintf(outputFile, "|");
        			else
        				fprintf(outputFile, " ");
        		fprintf(outputFile, "\n");
        		for (i = j; i > j - 50 && i >= 0; i--)
        			fprintf(outputFile, "%c", recon2[i]);
        		fprintf(outputFile, "\n\n");
        	}
        	fclose(outputFile);
        	free(recon1);
        	free(recon2);
        }
	free(start1);
	free(start2);
	free(compass);
    free(ix);
    free(iy);
    free(m);
    return score;
}

static PyObject	*needleman(PyObject* self, PyObject* args)
{
    char 		*s1;
    char 		*s2;
    int 		nEG;
    float 		dTh;
    int 		hPo;
    int 		gOp;
    int 		gEx;
    int 		dif;
	
    if (!PyArg_ParseTuple(args, "ssifiiii", &s1, &s2, &nEG, &dTh, &hPo, &gOp, &gEx, &dif))
        return NULL;
    return Py_BuildValue("f", _needleEmboss(s1, s2, nEG, gOp, gEx, gOp, gEx));
}

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

static PyObject *
error_out(PyObject *m) {
    struct module_state *st = GETSTATE(m);
    PyErr_SetString(st->error, "something bad happened");
    return NULL;
}

static PyMethodDef myextension_methods[] = {
    {"needleman", (PyCFunction)needleman, METH_VARARGS, "Calculate Needleman-Wunsch global alignment"},
    {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static int myextension_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int myextension_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "needleman",
        NULL,
        sizeof(struct module_state),
        myextension_methods,
        NULL,
        myextension_traverse,
        myextension_clear,
        NULL
};

#define INITERROR return NULL

PyObject *
PyInit_needleman_emboss(void)

#else
#define INITERROR return

void
PyInit_needleman_emboss(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("needleman", myextension_methods);
#endif

    if (module == NULL)
        INITERROR;
    struct module_state *st = GETSTATE(module);

    st->error = PyErr_NewException("myextension.Error", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
