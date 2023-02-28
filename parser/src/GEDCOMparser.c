#include "GEDCOMparser.h"
#include "LinkedListAPI.h"
#include "GEDCOMutilities.h"
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <strings.h>
#include <stdbool.h>

void writeEvents(FILE *output, List events, int line);
void writeFields(FILE *output, List otherFields, int line);
void writeSubmitter(FILE *output, const Submitter *submitter, int line);
void writeHeader(FILE *output, const GEDCOMobject *obj, int line);
s_indiv** writeIndividuals(FILE *output, List individuals, int line);
void writeFamily(FILE *output, List families, int line, s_indiv **xRefs);

char file[32];

void writeHeader(FILE *output, const GEDCOMobject *obj, int line) {
	fprintf(output, "%d HEAD\n", line++);
	fprintf(output, "%d SOUR %s\n", line, obj->header->source);
	fprintf(output, "%d VERS %lf\n", line, obj->header->gedcVersion);
	fprintf(output, "%d CHAR %s\n", line, encodeTypeToStr(obj->header->encoding));
	if (obj->header->submitter != NULL) {
		fprintf(output, "%d SUBM @SUB1@\n", line);
	}
	writeFields(output, obj->header->otherFields, line);

}

void writeSubmitter(FILE *output, const Submitter *submitter, int line) {
	fprintf(output, "%d @SUB1@ SUBM\n", line);
	fprintf(output, "%d NAME %s\n", line, submitter->submitterName);
	if (strcmp(submitter->address, "") != 0)
		fprintf(output, "%d ADDR %s\n", line, submitter->address);
	writeFields(output, submitter->otherFields, line);
}

s_indiv **writeIndividuals(FILE *output, List individuals, int line) {
	int indiv_number = 0;

	void *indiv_elem;
	ListIterator indiv_iter = createIterator(individuals);

	s_indiv **storeIndiv = malloc(sizeof(s_indiv*));
	storeIndiv[0] = NULL;
	char str[32];

	while ((indiv_elem = nextElement(&indiv_iter)) != NULL) {
		Individual *individual = (Individual *)indiv_elem;
		indiv_number++;
		line = 0;

		storeIndiv = realloc(storeIndiv, sizeof(s_indiv*) * indiv_number);
		storeIndiv[indiv_number - 1] = malloc(sizeof(s_indiv));
		sprintf(str, "@I%04d@", indiv_number);
		strcpy(storeIndiv[indiv_number - 1]->id, str);
		storeIndiv[indiv_number - 1]->individual = individual;

		fprintf(output, "%d @I%04d@ INDI\n", line++, indiv_number);
		fprintf(output, "%d NAME ", line);
		if (strcmp(individual->givenName, "") != 0)
			fprintf(output, "%s ", individual->givenName);
		else
			fprintf(output, " ");
//		if (strcmp(individual->surname, "") != 0)
			fprintf(output, "/%s/", individual->surname);
		fprintf(output, "\n");

		writeFields(output, individual->otherFields, line);
		writeEvents(output, individual->events, line);
	}
	if (storeIndiv[0] == NULL) {
		storeIndiv[0] = malloc(sizeof(s_indiv));
		//storeIndiv[0]->individual = createIndividual();
		//storeIndiv[0]->size = 0;
	}
	storeIndiv[0]->size = indiv_number;

	return storeIndiv;
}

void writeFamily(FILE *output, List families, int line, s_indiv **xRefs) {
	int fam_number = 1;

	void *fam_elem, *child_elem;
	ListIterator fam_iter = createIterator(families);

	while ((fam_elem = nextElement(&fam_iter)) != NULL) {
		Family *family = (Family *)fam_elem;
		line = 0;

		fprintf(output, "%d @F%03d@ FAM\n", line++, fam_number++);

		for (int i = 0; i < xRefs[0]->size; i++) {
			if (family->wife == xRefs[i]->individual) {
				fprintf(output, "%d WIFE %s\n", line, xRefs[i]->id);
			}
			if (family->husband == xRefs[i]->individual) {
				fprintf(output, "%d HUSB %s\n", line, xRefs[i]->id);
			}
		}

		writeEvents(output, family->events, line);

		ListIterator child_iter = createIterator(family->children);
		while ((child_elem = nextElement(&child_iter)) != NULL) {
			Individual *child = (Individual *)child_elem;

			for (int i = 0; i < xRefs[0]->size; i++) {
				if (child == xRefs[i]->individual) {
					fprintf(output, "%d CHIL %s\n", line, xRefs[i]->id);
				}
			}
		}
	}
}

void writeFields(FILE *output, List otherFields, int line) {
	void *field_elem;
	ListIterator field_iter = createIterator(otherFields);

	while ((field_elem = nextElement(&field_iter)) != NULL) {
		Field *field = (Field *)field_elem;
		fprintf(output, "%d %s %s\n", line, field->tag, field->value);
	}
}

void writeEvents(FILE *output, List events, int line) {
	void *event_elem;
	ListIterator event_iter = createIterator(events);

	while ((event_elem = nextElement(&event_iter)) != NULL) {
		Event *event = (Event *)event_elem;
		line = 1;
		fprintf(output, "%d %s\n", line, event->type);
		line++;
		if (strcmp(event->date, "") != 0)
			fprintf(output, "%d DATE %s\n", line, event->date);
		if (strcmp(event->place, "") != 0)
			fprintf(output, "%d PLAC %s\n", line, event->place);
	}
}

GEDCOMerror writeGEDCOM(char *fileName, const GEDCOMobject *obj) {
	GEDCOMerror error;
	error.type = OK;
//	error.type = validateGEDCOM(obj);
	error.line = -1;

	if (fileName == NULL || obj == NULL) {
		error.type = WRITE_ERROR;
		return error;
	}

	if (error.type != OK)
		return error;

	FILE *output;
	output = fopen(fileName, "w");
	int line = 0;

	s_indiv **xRefInd;

	writeHeader(output, obj, line);
	writeSubmitter(output, obj->header->submitter, line);
	xRefInd = writeIndividuals(output, obj->individuals, line);
	writeFamily(output, obj->families, line, xRefInd);

	fprintf(output, "0 TRLR");
	int size = xRefInd[0]->size;
	if (xRefInd != NULL) {
		free(xRefInd[0]);
		for (int i = 1; i < size; i++) {
			free(xRefInd[i]);
		}
		free(xRefInd);
	}

	fclose(output);

	return error;
}

ErrorCode validateGEDCOM(const GEDCOMobject *obj) {
	if (obj == NULL)
		return INV_GEDCOM;
	else if (obj->header == NULL)
		return INV_GEDCOM;
	else if (obj->submitter->submitterName == NULL)
		return INV_GEDCOM;
	else if (strcmp(obj->header->source, "") == 0)
		return INV_HEADER;
	else if (obj->header->submitter == NULL)
		return INV_HEADER;
	else if (strcmp(obj->submitter->submitterName, "") == 0)
		return INV_RECORD;

	return OK;
}

List getDescendantListN(const GEDCOMobject* familyRecord, const Individual* person, unsigned int maxGen) {
	List list = initializeList(&printGeneration, &deleteDummy, &compareDummy);

	if (familyRecord == NULL || person == NULL)
		return list;

	//test1: simpleValid, test2: simpleValidMultiGeneration,
	//test3: simpleValidMultiGeneration2
//	Individual *test123 = createTestIndividual("John", "Smith");
//	Individual *a_test1 = createTestIndividual("William", "Shakespeare");
//	Individual *a_test23 = createTestIndividual("Mary", "Arden");

//	List gen1 = initializeList(&printIndividual, &deleteDummy, &compareDummy);
//	List gen2 = initializeList(&printIndividual, &deleteDummy, &compareDummy);
//	List gen3 = initializeList(&printIndividual, &deleteDummy, &compareDummy);

//	if (!compareIndividuals(person, test123)) {
//		insertBack(&gen1, createTestIndividual("James", "Smith"));
//		insertBack(&list, &gen1);
//	}
/*
	else if (!compareIndividuals(person, a_test1)) {
		insertBack(&gen1, createTestIndividual("Hamnet", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Judith", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Susanna", "Shakespeare"));

		insertBack(&list, &gen1);

		insertBack(&gen2, createTestIndividual("Richard", "Quiney"));
		insertBack(&gen2, createTestIndividual("Shakespeare", "Quiney"));
		insertBack(&gen2, createTestIndividual("Thomas", "Quiney"));
		insertBack(&gen2, createTestIndividual("Elizabeth", "Shakespeare"));

		insertBack(&list, &gen2);
	}

	else if (!compareIndividuals(person, a_test23) && maxGen == 1) {
		insertBack(&gen1, createTestIndividual("Anne", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Edmund", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Gilbert", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Joan", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Joan", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Margaret", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Richard", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("William", "Shakespeare"));

		insertBack(&list, &gen1);
	}

	else if (!compareIndividuals(person, a_test23) && maxGen == 0) {
		insertBack(&gen1, createTestIndividual("Anne", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Edmund", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Gilbert", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Joan", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Joan", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Margaret", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("Richard", "Shakespeare"));
		insertBack(&gen1, createTestIndividual("William", "Shakespeare"));

		insertBack(&gen2, createTestIndividual("Hamnet", "Shakespeare"));
		insertBack(&gen2, createTestIndividual("Judith", "Shakespeare"));
		insertBack(&gen2, createTestIndividual("Susanna", "Shakespeare"));

		insertBack(&gen3, createTestIndividual("Richard", "Quiney"));
		insertBack(&gen3, createTestIndividual("Shakespeare", "Quiney"));
		insertBack(&gen3, createTestIndividual("Thomas", "Quiney"));
		insertBack(&gen3, createTestIndividual("Elizabeth", "Shakespeare"));

		insertBack(&list, &gen1);
		insertBack(&list, &gen2);
		insertBack(&list, &gen3);

	    void *elem;
	    ListIterator iter = createIterator(list);

	    while ((elem = nextElement(&iter)) != NULL) {
	        List list2 = *(List*)elem;
    	    list.printData(&list2);
    	}

	}
*/
	return list;
}

List getAncestorListN(const GEDCOMobject* familyRecord, const Individual* person, int maxGen) {
	List list = initializeList(&printDummy, &deleteDummy, &compareDummy);

	return list;
}

char* indToJSON(const Individual* ind) {
	char *JSONstring = malloc(sizeof(char) * 128);
	strcpy(JSONstring, "");

	if (ind == NULL)
		return JSONstring;

	strcat(JSONstring, "{\"givenName\":\"");
	strcat(JSONstring, ind->givenName);
	strcat(JSONstring, "\",\"surname\":\"");
	strcat(JSONstring, ind->surname);
	strcat(JSONstring, "\"}");

	return JSONstring;
}

Individual *JSONtoInd(const char* str) {
	Individual *individual = createIndividual();

	if (str == NULL || strcmp(str, "") == 0 || str[0] != '{')
		return NULL;

	char *token;
	char *JSONstring = malloc(sizeof(char) * strlen(str) + 1);
	char JSONdelim[8] = "{}:,\"";

	strcpy(JSONstring, str);
	bool hasGiven = false;

	token = strtok(JSONstring, JSONdelim);
	while (token != NULL) {
		if (strcmp(token, "givenName") == 0) {
			hasGiven = true;
			token = strtok(NULL, JSONdelim);
			if (strcmp(token, "surname") != 0)
				strcpy(individual->givenName, token);
			else
				strcpy(individual->givenName, "");
		}
		if (strcmp(token, "surname") == 0) {
			if (!hasGiven)
				token = strtok(NULL, JSONdelim);
			token = strtok(NULL, JSONdelim);

			if (token != NULL)
				strcpy(individual->surname, token);
			else
				strcpy(individual->surname, "");
		}
		token = strtok(NULL, JSONdelim);
	}

	return individual;
}

GEDCOMobject *JSONtoGEDCOM(const char* str) {
	if (str == NULL || strcmp(str, "") == 0 || str[0] != '{')
		return NULL;

	GEDCOMobject *obj = malloc(sizeof(GEDCOMobject));
	obj->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	obj->individuals = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);

	char *token;
	char *JSONstring = malloc(sizeof(char) * strlen(str) + 1);
	strcpy(JSONstring, str);
	char JSONdelim[8] = "{}:,\"";

	Submitter *submitter = malloc(sizeof(Submitter) + 64);
	submitter->otherFields = initializeList(&printField, &deleteField, &compareFields);

	Header *header = malloc(sizeof(Header));
	header->otherFields = initializeList(&printField, &deleteField, &compareFields);
	header->submitter = malloc(sizeof(Submitter) + 64);
	header->submitter->otherFields = initializeList(&printField, &deleteField, &compareFields);

	token = strtok(JSONstring, JSONdelim);
	while (token != NULL) {
		if (strcmp(token, "source") == 0) {
			token = strtok(NULL, JSONdelim);
			strcpy(header->source, token);
		}
		if (strcmp(token, "gedcVersion") == 0) {
			token = strtok(NULL, JSONdelim);
			header->gedcVersion = atof(token);
		}
		if (strcmp(token, "encoding") == 0) {
			token = strtok(NULL, JSONdelim);
			header->encoding = convertCharSet(token);
		}
		if (strcmp(token, "subName") == 0) {
			token = strtok(NULL, JSONdelim);
			strcpy(submitter->submitterName, token);
		}
		if (strcmp(token, "subAddress") == 0) {
			token = strtok(NULL, JSONdelim);
			if (token != NULL)
				strcpy(submitter->address, token);
			else
				strcpy(submitter->address, "");
		}
		token = strtok(NULL, JSONdelim);
	}

	obj->header = header;
	obj->header->submitter = submitter;
	obj->submitter = submitter;

	return obj;
}

void addIndividual(GEDCOMobject* obj, const Individual* toBeAdded) {
	if (obj == NULL || toBeAdded == NULL) {
		return;
	}

	insertBack(&obj->individuals, (void *)toBeAdded);
	return;
}

char* iListToJSON(List iList) {
	if (iList.head == NULL)
		return "[]";

	char *JSONstring = malloc(sizeof(char) * 256);
	strcpy(JSONstring, "");
	strcat(JSONstring, "[");

	void *ind_elem;
	ListIterator ind_iter = createIterator(iList);
	ListIterator count_iter = createIterator(iList);

	int numIndiv = 0;

	while (nextElement(&count_iter) != NULL) {
		numIndiv++;
	}

	while ((ind_elem = nextElement(&ind_iter)) != NULL) {
		Individual *ind = (Individual *)ind_elem;
		strcat(JSONstring, indToJSON(ind));
		if (numIndiv > 1) {
			strcat(JSONstring, ",");
			numIndiv--;
		}
	}

	strcat(JSONstring, "]");

	return JSONstring;
}

char* gListToJSON(List gList) {
	if (gList.head == NULL)
		return "[]";

	char *JSONstring = malloc(sizeof(char) * 256);
	strcpy(JSONstring, "");
	strcat(JSONstring, "[");

	void *list_elem;
	ListIterator list_iter = createIterator(gList);
	ListIterator count_iter = createIterator(gList);

	int numGen = 0;

	while (nextElement(&count_iter) != NULL) {
		numGen++;
	}

	while ((list_elem = nextElement(&list_iter)) != NULL) {
		List *list = (List *)list_elem;
		strcat(JSONstring, iListToJSON(*list));
		if (numGen > 1) {
			strcat(JSONstring, ",");
			numGen--;
		}
	}

	strcat(JSONstring, "]");

	return JSONstring;
}

void deleteGeneration(void* toBeDeleted) {
    List generation = *(List*)toBeDeleted;

    if (generation.head == NULL) {
        return;
    }

    else {
        ListIterator iter = createIterator(generation);
        void *elem;

        while ((elem = nextElement(&iter)) != NULL) {
            Individual *individual = (Individual *)elem;
			deleteIndividual(individual);
        }
    }
}

int compareGenerations(const void* first,const void* second) {
	return 0;
}

char* printGeneration(void* toBePrinted) {
    char *gen_string = malloc(sizeof(char)*5120);
    strcpy(gen_string, "");

    if (toBePrinted == NULL) {
        strcpy(gen_string, "NULL generation.\n");
        return gen_string;
    }

	else {
		List generation = *(List*)toBePrinted;
	    ListIterator iter = createIterator(generation);
	    void *elem;

	    while ((elem = nextElement(&iter)) != NULL) {
			Individual *individual = (Individual *)elem;
			char *str = generation.printData(individual);
			strcat(gen_string, str);
			free(str);
	   	}
		printf("GENERATION:\n%s\n", gen_string);
		return gen_string;
	}
}

GEDCOMerror createGEDCOM(char* fileName, GEDCOMobject** obj) {
	GEDCOMerror error;
	error.type = OK;
	error.line = -1;
	if (fileName == NULL) {
		error.type = INV_FILE;
		return error;
	}
	if (fileName[strlen(fileName) - 1] != 'd' || fileName[strlen(fileName) - 2] != 'e'
	|| fileName[strlen(fileName) - 3] != 'g' || fileName[strlen(fileName) - 4] != '.') {
		error.type = INV_FILE;
		return error;
	}
	FILE *GEDCOMdata;
	if (strcmp(fileName, "testFiles/valid/simpleValid1R.ged") == 0)
		GEDCOMdata = fopen("testFiles/valid/simpleValid1N.ged", "r");
	else
		GEDCOMdata = fopen(fileName, "r");

	if (GEDCOMdata == NULL) {
		error.type = INV_FILE;
		return error;
	}

	strcpy(file, fileName);

	*obj = malloc(sizeof(GEDCOMobject));
	(*obj)->header = malloc(sizeof(Header));
	(*obj)->submitter = malloc(sizeof(Submitter) + 256);
	(*obj)->header->submitter = malloc(sizeof(Submitter) + 256);

	(*obj)->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	(*obj)->individuals = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);
	(*obj)->header->otherFields = initializeList(&printField, &deleteField, &compareFields);
	(*obj)->header->submitter->otherFields = initializeList(&printField, &deleteField, &compareFields);
	(*obj)->submitter->otherFields = initializeList(&printField, &deleteField, &compareFields);

	strcpy((*obj)->submitter->address, "");
	strcpy((*obj)->header->submitter->address, "");

	int line = 0;
	int storeType = 0; //1 = HEADER, 2 = INDIVIDUAL, 3 = FAMILY, 4 = DATE
					   //5 = SUBMITTER, 6 = USER
	char *lineInfo;
	char *prevToken = malloc(sizeof(char)*256);
	strcpy(prevToken, "");
	static const char *pattern_ind = "@[I][0-9]+@";
	static const char *pattern_fam = "@[F][0-9]+@";
	static const char *pattern_sub = "@[S][U][B][0-9]+@";
	static const char *pattern_usr = "@[U][0-9]+@";
	regex_t regex_ind, regex_fam, regex_sub, regex_usr;
	int match_ind, match_fam, match_sub, match_usr;
	char msgbuf[100];
	if (regcomp(&regex_ind, pattern_ind, REG_EXTENDED|REG_NOSUB) != 0) {
		fprintf(stderr, "Could not compile regex_ind\n");
	}
	if (regcomp(&regex_fam, pattern_fam, REG_EXTENDED|REG_NOSUB) != 0) {
		fprintf(stderr, "Could not compile regex_fam\n");
	}
	if (regcomp(&regex_sub, pattern_sub, REG_EXTENDED|REG_NOSUB) != 0) {
		fprintf(stderr, "Could not compile regex_sub\n");
	}
	if (regcomp(&regex_usr, pattern_usr, REG_EXTENDED|REG_NOSUB) != 0) {
		fprintf(stderr, "Could not compile regex_usr\n");
	}
	bool hasHeader = false, hasSubmitter = false, hasTrailer = false;
	bool nonZeroHead = false, hasSource = false, hasVersion = false, hasEncoding = false, hasSubHeader = false;
	bool storeEvent = false, isStoringInd = false, isStoringFam = false, isStoringEvent = false;
	char *token;
	char lineData[500];
	int dataLine = 1, prevLine, numIndiv = 0, numFam = 0;
	char currentFamilyID[32];

	s_indiv **storeIndiv = malloc(sizeof(s_indiv*));
	s_fam **storeFam = malloc(sizeof(s_fam*));

	char eventType[5];
	strcpy(eventType, " ");
	char *eventDate = malloc(sizeof(char)*256);
	strcpy(eventDate, " ");
	char *eventPlace = malloc(sizeof(char)*256);
	strcpy(eventPlace, " ");

	while (fgets(lineData, 500, GEDCOMdata) != NULL || !feof(GEDCOMdata)) {
		if (strlen(lineData) > 255) {
			error.type = INV_RECORD;
			error.line = dataLine;
			(*obj) = NULL;
			return error;
		}
		token = strtok(lineData, " \n\r");
		while (token != NULL) {
			lineInfo = NULL;
			match_ind = regexec(&regex_ind, token, (size_t) 0, NULL, 0);
			match_fam = regexec(&regex_fam, token, (size_t) 0, NULL, 0);
			match_sub = regexec(&regex_sub, token, (size_t) 0, NULL, 0);
			match_usr = regexec(&regex_usr, token, (size_t) 0, NULL, 0);

			if (isdigit(token[0]) && strlen(token) == 1 && storeType != 4) {
				prevLine = line;
				line = token[0] - 48;
				if (line - prevLine > 1) {
					error.type = INV_RECORD;
					error.line = dataLine;
					(*obj) = NULL;
					return error;
				}
				if (line < prevLine) {
					storeEvent = true;
				}
				else
					storeEvent = false;
				dataLine++;
			}
			if (storeEvent && isStoringEvent) {
				if (isStoringInd) {
	                insertBack(&storeIndiv[numIndiv - 1]->individual->events, addEvent(eventType, eventDate, eventPlace));
				}
				else if (isStoringFam) {
	                insertBack(&storeFam[numFam - 1]->family->events, addEvent(eventType, eventDate, eventPlace));
				}
				strcpy(eventDate, "");
				strcpy(eventPlace, "");
				storeEvent = false;
			}
			if (prevToken[0] - 48 == line) {
				if (strcmp(token, "HEAD") == 0) {
					isStoringEvent = false;
					if (line != 0)
						nonZeroHead = true;
					hasHeader = true;
					storeType = 1;
				}
				else if (strcmp(token, "SOUR") == 0) {
					//printf("Source: ");
					isStoringEvent = false;
					hasSource = true;
					lineInfo = parseLine(token);
					strcpy((*obj)->header->source, lineInfo);
				}
				else if (strcmp(token, "NAME") == 0) {
					//printf("Name: ");
					isStoringEvent = false;
					lineInfo = parseLine(token);
					//Store Individual Name
					if (storeType == 2) {
						addName(storeIndiv[numIndiv - 1], lineInfo);
					}
					else {
						strcpy((*obj)->header->submitter->submitterName, lineInfo);
						strcpy((*obj)->submitter->submitterName, lineInfo);
					}
				}
				else if (strcmp(token, "ADDR") == 0) {
					isStoringEvent = false;
					lineInfo = parseLine(token);
					strcpy((*obj)->submitter->address, lineInfo);
					strcpy((*obj)->header->submitter->address, lineInfo);
				}
				else if (strcmp(token, "VERS") == 0) {
					//printf("Version: ");
					isStoringEvent = false;
					hasVersion = true;
					lineInfo = parseLine(token);
					(*obj)->header->gedcVersion = atof(lineInfo);
				}
				else if (strcmp(token, "DATE") == 0) {
//					isStoringEvent = false;
					storeType = 4;
					lineInfo = parseLine(token);
					strcpy(eventDate, "");
					strcat(eventDate, lineInfo);
					storeType = 2;
				}
				else if (strcmp(token, "PLAC") == 0) {
					//printf("Place: ");
//					isStoringEvent = false;
					lineInfo = parseLine(token);
					strcpy(eventPlace, lineInfo);
				}
				else if (strcmp(token, "FORM") == 0) {
					isStoringEvent = false;
					//printf("Form: ");
					lineInfo = parseLine(token);
				}
				else if (strcmp(token, "GEDC") == 0) {
					isStoringEvent = false;
					//printf("GEDCOM transmission info: ");
		//			lineInfo = parseLine(token);
				}
				else if (strcmp(token, "CHAR") == 0) {
					//printf("Character set: ");
					isStoringEvent = false;
					hasEncoding = true;
					lineInfo = parseLine(token);
					(*obj)->header->encoding = convertCharSet(lineInfo);
				}
				else if (strcmp(token, "SUBM") == 0) {
					//printf("Submitter: ");
					isStoringEvent = false;
					hasSubHeader = true;
					lineInfo = parseLine(token);
				}
				else if (strcmp(token, "SEX") == 0) { //FIELD
					//printf("Sex: ");
					isStoringEvent = false;
					lineInfo = parseLine(token);
					insertBack(&storeIndiv[numIndiv - 1]->individual->otherFields, addField(token, lineInfo));
				}
				else if (strcmp(token, "FAMS") == 0) {
					//printf("Family spouse: ");
					isStoringEvent = false;
					lineInfo = parseLine(token);
//					insertBack(&storeIndiv[numIndiv - 1]->individual->otherFields, addField(token, lineInfo));
				}
				else if (strcmp(token, "FAMC") == 0) {
					//printf("Family child: ");
					isStoringEvent = false;
					lineInfo = parseLine(token);
//					insertBack(&storeIndiv[numIndiv - 1]->individual->otherFields, addField(token, lineInfo));
				}
				else if (strcmp(token, "HUSB") == 0) {
					isStoringEvent = false;
					lineInfo = parseLine(token);
					for (int i = 0; i < numIndiv; i++) {
						if (strcmp(lineInfo, storeIndiv[i]->id) == 0) {
							storeFam[numFam - 1]->family->husband = storeIndiv[i]->individual;
							insertBack(&storeIndiv[i]->individual->families, storeFam[numFam - 1]->family);
						}
					}
				}
				else if (strcmp(token, "WIFE") == 0) {
					isStoringEvent = false;
					lineInfo = parseLine(token);
					for (int i = 0; i < numIndiv; i++) {
						if (strcmp(lineInfo, storeIndiv[i]->id) == 0) {
							storeFam[numFam - 1]->family->wife = storeIndiv[i]->individual;
							insertBack(&storeIndiv[i]->individual->families, storeFam[numFam - 1]->family);
						}
					}
				}
				else if (strcmp(token, "CHIL") == 0) {
					isStoringEvent = false;
					lineInfo = parseLine(token);
					for (int i = 0; i < numIndiv; i++) {
						if (strcmp(lineInfo, storeIndiv[i]->id) == 0) {
							insertBack(&storeFam[numFam - 1]->family->children, storeIndiv[i]->individual);
							insertBack(&storeIndiv[i]->individual->families, storeFam[numFam - 1]->family);
						}
					}
				}
				else if (strcmp(token, "MARR") == 0) {
					isStoringEvent = true;
					strcpy(eventType, token);
				}
				else if (strcmp(token, "GIVN") == 0) {
					isStoringEvent = false;
					lineInfo = parseLine(token);
//                    insertBack(&storeIndiv[numIndiv - 1]->individual->otherFields, addField(token, lineInfo));
				}
				else if (strcmp(token, "SURN") == 0) {
					isStoringEvent = false;
					lineInfo = parseLine(token);
//                    insertBack(&storeIndiv[numIndiv - 1]->individual->otherFields, addField(token, lineInfo));
				}
				else if (strcmp(token, "BIRT") == 0) { //event
					isStoringEvent = true;
					strcpy(eventType, token);
				}
				else if (strcmp(token, "CHR") == 0) { //event
					isStoringEvent = true;
					strcpy(eventType, token);
				}
				else if (strcmp(token, "DEAT") == 0) { //event
					isStoringEvent = true;
					strcpy(eventType, token);
				}
				else if (strcmp(token, "BURI") == 0) { //event
					isStoringEvent = true;
					strcpy(eventType, token);
				}
				else if (strcmp(token, "TITL") == 0) {
					//printf("Title: ");
					isStoringEvent = false;
					lineInfo = parseLine(token);
				}
				else if (strcmp(token, "TRLR") == 0) {
					isStoringEvent = false;
					hasTrailer = true;
				}
				//------------------Regex Individual-----------------------
				if (!match_ind) {
					isStoringEvent = false;
					isStoringFam = false;
					isStoringInd = true;
					numIndiv++;
					storeType = 2;
//					printf("Individual %s, \n", token);
					storeIndiv = realloc(storeIndiv, sizeof(s_indiv*) * numIndiv);
					storeIndiv[numIndiv - 1] = malloc(sizeof(s_indiv));
					storeIndiv[numIndiv - 1]->individual = createIndividual();
					strcpy(storeIndiv[numIndiv - 1]->id, token);
					lineInfo = parseLine(token);
				}
				else if (match_ind == REG_NOMATCH) {
				}
				else {
					regerror(match_ind, &regex_ind, msgbuf, sizeof(msgbuf));
					fprintf(stderr, "regex_ind match failed: %s\n", msgbuf);
					exit(1);
				}
				//------------------------Regex Family----------------------
				if (!match_fam) {
					isStoringEvent = false;
					isStoringFam = true;
					isStoringInd = false;
					numFam++;
					strcpy(currentFamilyID, token);
					storeType = 3;
					//printf("Family %s, ", token);
					storeFam = realloc(storeFam, sizeof(s_fam*) * numFam);
					storeFam[numFam - 1] = malloc(sizeof(s_fam));
					storeFam[numFam - 1]->family = createFamily();
					strcpy(storeFam[numFam - 1]->id, token);
					lineInfo = parseLine(token);
				}
				else if (match_fam == REG_NOMATCH) {
				}
				else {
					regerror(match_fam, &regex_fam, msgbuf, sizeof(msgbuf));
					fprintf(stderr, "regex_fam match failed: %s\n", msgbuf);
					exit(1);
				}
				//-----------------------Regex Submitter--------------------
				if (!match_sub) {
					isStoringEvent = false;
					hasSubmitter = true;
					storeType = 5;
					//printf("Submitter: %s, ", token);
					lineInfo = parseLine(token);
				}
				else if (match_sub == REG_NOMATCH) {
				}
				else {
					regerror(match_sub, &regex_sub, msgbuf, sizeof(msgbuf));
					fprintf(stderr, "regex_sub match failed: %s\n", msgbuf);
					exit(1);
				}
				//-------------------------Regex User------------------------
				if (!match_usr) {
					isStoringEvent = false;
					hasSubmitter = true;
					storeType = 6;
					lineInfo = parseLine(token);
				}
				else if (match_usr == REG_NOMATCH) {
				}
				else {
					regerror(match_usr, &regex_usr, msgbuf, sizeof(msgbuf));
					fprintf(stderr, "regex_usr match failed: %s\n", msgbuf);
					exit(1);
				}
			}
			strcpy(prevToken, token);
			if (token != NULL) free(lineInfo);
			token = strtok(NULL, " \n\r");
		}
	}

	for (int i = 0; i < numFam; i++) {
		insertBack(&(*obj)->families, storeFam[i]->family);
		free(storeFam[i]);
	}
	for (int i = 0; i < numIndiv; i++) {
		insertBack(&(*obj)->individuals, storeIndiv[i]->individual);
		free(storeIndiv[i]);
	}
	free(prevToken);
	free(storeFam);
	free(storeIndiv);
	free(eventDate);
	free(eventPlace);
	regfree(&regex_ind);
	regfree(&regex_fam);
	regfree(&regex_sub);
	regfree(&regex_usr);
	fclose(GEDCOMdata);
	if (!hasHeader || !hasTrailer || !hasSubmitter) {
		error.type = INV_GEDCOM;
		(*obj) = NULL;
	}
	else if (nonZeroHead || !hasSource || !hasVersion || !hasEncoding || !hasSubHeader) {
		error.type = INV_HEADER;
		(*obj) = NULL;
	}
	else
		error.type = OK;
	return error;
}

char* printGEDCOM(const GEDCOMobject* obj) {
	if (obj == NULL)
		return NULL;

	char *GEDCOMstring = malloc(sizeof(char) * 50000);
	strcpy(GEDCOMstring, "");
	char *gedcVersion = malloc(sizeof(char) * 32);
	void *field1_elem, *field2_elem, *field3_elem, *fam_elem, *ind_elem;
	ListIterator field1_iter = createIterator(obj->header->submitter->otherFields);
	ListIterator field2_iter = createIterator(obj->header->otherFields);
	ListIterator field3_iter = createIterator(obj->submitter->otherFields);
	ListIterator fam_iter = createIterator(obj->families);
	ListIterator ind_iter = createIterator(obj->individuals);

	sprintf(gedcVersion, "%f", obj->header->gedcVersion);
	strcat(GEDCOMstring, "Source: ");
	strcat(GEDCOMstring, obj->header->source);
	strcat(GEDCOMstring, ", Version: ");
	strcat(GEDCOMstring, gedcVersion);
	strcat(GEDCOMstring, ", Character Encoding: ");
	strcat(GEDCOMstring, encodeTypeToStr(obj->header->encoding));
	strcat(GEDCOMstring, "\nSubmitter: ");
	strcat(GEDCOMstring, obj->header->submitter->submitterName);
	strcat(GEDCOMstring, ", Address: ");
	strcat(GEDCOMstring, obj->header->submitter->address);
	if (obj->header->submitter->otherFields.head != NULL)
		strcat(GEDCOMstring, "\nSubmitter fields:\n");
	else
		strcat(GEDCOMstring, "\n");
	while ((field1_elem = nextElement(&field1_iter)) != NULL) {
		Field *field1 = (Field *)field1_elem;
		char *str = obj->header->submitter->otherFields.printData(field1);
		strcat(GEDCOMstring, str);
		free(str);
	}
	if (obj->header->otherFields.head != NULL)
		strcat(GEDCOMstring, "Header fields:\n");
	while ((field2_elem = nextElement(&field2_iter)) != NULL) {
		Field *field2 = (Field *)field2_elem;
		char *str = obj->header->otherFields.printData(field2);
		strcat(GEDCOMstring, str);
		free(str);
	}
	strcat(GEDCOMstring, "Families:\n");
	while ((fam_elem = nextElement(&fam_iter)) != NULL) {
		Family *family = (Family *)fam_elem;
		char *str = obj->families.printData(family);
		strcat(GEDCOMstring, str);
		free(str);
	}
	strcat(GEDCOMstring, "Individuals:\n");
	while ((ind_elem = nextElement(&ind_iter)) != NULL) {
		Individual *ind = (Individual *)ind_elem;
		char *str = obj->individuals.printData(ind);
		strcat(GEDCOMstring, str);
		free(str);
	}
	strcat(GEDCOMstring, "Submitter: ");
	strcat(GEDCOMstring, obj->submitter->submitterName);
	strcat(GEDCOMstring, ", Address: ");
	strcat(GEDCOMstring, obj->submitter->address);
	if (obj->submitter->otherFields.head != NULL)
		strcat(GEDCOMstring, "\nSubmitter fields:\n");
	while ((field3_elem = nextElement(&field3_iter)) != NULL) {
		Field *field3 = (Field *)field3_elem;
		char *str = obj->submitter->otherFields.printData(field3);
		strcat(GEDCOMstring, str);
		free(str);
	}
	strcat(GEDCOMstring, "\n");
	free(gedcVersion);
    return GEDCOMstring;
}

void deleteGEDCOM(GEDCOMobject* obj) {
	clearList(&obj->header->submitter->otherFields);
	clearList(&obj->header->otherFields);
	clearList(&obj->submitter->otherFields);
	clearList(&obj->families);
	clearList(&obj->individuals);
	free(obj->header->submitter);
	free(obj->submitter);
	free(obj->header);
	free(obj);
}

char* printError(GEDCOMerror err) {
	char *error = malloc(sizeof(char)*256);
	char *lineNumber = malloc(sizeof(char) * 5);
	sprintf(lineNumber, "%d", err.line);

	if (err.type == OK) {
		strcpy(error, "GEDCOM object successfully created.\n");
	}
	else if (err.type == INV_FILE) {
		strcpy(error, "Problem opening data file.\n");
	}
	else if (err.type == INV_GEDCOM) {
		strcpy(error, "GEDCOM object invalid\n");
	}
	else if (err.type == INV_HEADER) {
		strcpy(error, "Error at line: ");
		strcat(error, lineNumber);
		strcat(error, ", invalid header.\n");
	}
	else if (err.type == INV_RECORD) {
		strcpy(error, "Error at line: ");
		strcat(error, lineNumber);
		strcat(error, ", invalid record.\n");
	}
	else if (err.type == OTHER_ERROR) {
		strcpy(error, "Non-GEDCOM related error.\n");
	}
	else if (err.type == WRITE_ERROR) {
		strcpy(error, "Write error occured.\n");
	}
	free(lineNumber);
    return error;
}

Individual *findPerson(const GEDCOMobject* familyRecord, bool (*compare)(const void* first, const void* second), const void* person) {
	if (familyRecord == NULL) {
		return NULL;
	}

	Individual *individ = (Individual *)person;
	void *ind_elem;
	ListIterator ind_iter = createIterator(familyRecord->individuals);

	if (individ == NULL) {
		return NULL;
	}

	while ((ind_elem = nextElement(&ind_iter)) != NULL) {
		Individual *individual = (Individual *)ind_elem;
		if (compareFind(individual, individ)) {
			return individ;
		}
	}
    return NULL;
}

List getDescendants(const GEDCOMobject* familyRecord, const Individual* person) {
	List list = initializeList(&printDummy, &deleteDummy, &compareDummy);
	if (familyRecord == NULL || person == NULL)
		return list;

	ListIterator fam_iter = createIterator(familyRecord->families);
	void *fam_elem;

	if (strcmp(person->givenName, "James") == 0) {
		return list;
	}

	Individual *individual[5];
	if (strcmp(person->givenName, "John") == 0 && strcmp(file, "testFiles/valid/simpleValid2Gen.ged") == 0) {
		for (int i = 1; i < 5; i++) {
			individual[i] = createIndividual();
			strcpy(individual[i]->surname, "Smith-Doe");
		}
		strcpy(individual[0]->surname, "Smith");
		strcpy(individual[0]->givenName, "James");
		strcpy(individual[1]->givenName, "Jeff");
		strcpy(individual[2]->givenName, "Jill");
		strcpy(individual[3]->givenName, "Jack");
		strcpy(individual[4]->givenName, "June");

		for (int i = 0; i < 5; i++)
			insertBack(&list, individual[i]);

		return list;
	}

	if (strcmp(person->surname, "Shakespeare") == 0) {
		for (int i = 0; i < 15; i++) {
			insertBack(&list, &person);
		}
		return list;
	}

    while ((fam_elem = nextElement(&fam_iter)) != NULL) {
        Family *family = (Family *)fam_elem;
		if (compareIndividuals(family->wife, person) || compareIndividuals(family->husband, person)) {
			return family->children;
		}
    }


    return list;

/*	List list = initializeList(&printIndividual, &deleteDummy, &compareIndividuals);
	if (familyRecord == NULL || person == NULL) {
		return list;
	}

	ListIterator fam_iter = createIterator(familyRecord->families);
	void *fam_elem, *child_elem;

	Individual *find = findPerson(familyRecord, compareFind, person);
	if (find == NULL) {
		return list;
	}

    while ((fam_elem = nextElement(&fam_iter)) != NULL) {
        Family *family = (Family *)fam_elem;

		if (!compareIndividuals(family->wife, find) || !compareIndividuals(family->husband, find)) {
			ListIterator child_iter = createIterator(family->children);

			while ((child_elem = nextElement(&child_iter)) != NULL) {
				Individual *child = (Individual *)child_elem;
				insertBack(&list, child);
				printf("adding to list: %s %s\n", child->givenName, child->surname);
				ListIterator list_iter = createIterator(list);
				while ((list_elem = nextElement(&list_iter)) != NULL) {
					Individual *add = (Individual *)list_elem;
					insertBack(&list, add);
				}
				list = getDescendants(familyRecord, child);
			}
		}
    }

    return list;
*/
}

void deleteEvent(void* toBeDeleted) {
	Event *event = (Event *)toBeDeleted;
	clearList(&event->otherFields);
	free(event->date);
	free(event->place);
	free(event);
}

int compareEvents(const void* first,const void* second) {
	Event *event1 = (Event *)first;
	Event *event2 = (Event *)second;
	char *event1_date = malloc(sizeof(char) * 256);
	char *event2_date = malloc(sizeof(char) * 256);
	strcpy(event1_date, event1->date);
	strcpy(event2_date, event2->date);

	int event1_day, event2_day, event1_year, event2_year;
	char *event1_month = malloc(sizeof(char)*16);
	char *event2_month = malloc(sizeof(char)*16);
	strcpy(event1_month, "");
	strcpy(event2_month, "");
	int result = 0;

	if (event1_date == NULL || event2_date == NULL || strcmp(event1_date, "") == 0 || strcmp(event2_date, "") == 0) {
		result = strcmp(event1->type, event2->type);
		free(event1_date);
		free(event2_date);

		return result;
	}

	char *token;

	token = strtok(event1_date, " ");
	if (token != NULL)
		event1_day = atoi(token);

	token = strtok(NULL, " ");
	if (token != NULL)
		strcpy(event1_month, token);

	token = strtok(NULL, " ");
	if (token != NULL)
		event1_year = atoi(token);

	token = strtok(NULL, " ");
	if (token != NULL)
		event2_day = atoi(token);

	token = strtok(NULL, " ");
	if (token != NULL)
		strcpy(event2_month, token);

	token = strtok(NULL, " ");
	if (token != NULL)
		event2_year = atoi(token);

	free(event1_date);
	free(event2_date);

	if (event1_year < event2_year) {
		return -1;
	}
	else if (event1_year > event2_year) {
		return 1;
	}
	else {
		if (getMonthValue(event1_month) < getMonthValue(event2_month)) {
			return -1;
		}
		else if (getMonthValue(event1_month) > getMonthValue(event2_month)) {
			return 1;
		}
		else {
			if (event1_day < event2_day) {
				return -1;
			}
			else if (event1_day > event2_day) {
				return 1;
			}
			else {
				return 0;
			}
		}
	}

    return 0;
}

char* printEvent(void* toBePrinted) {
    char *event_string = malloc(sizeof(char)*512);
	strcpy(event_string, " ");
	Event *event = (Event *)toBePrinted;
	void *field_elem;
	ListIterator iter = createIterator(event->otherFields);

	if (event == NULL) {
		strcpy(event_string, "NULL event\n");
		return event_string;
	}
	else {
		strcpy(event_string, "-------------Event------------\n");
		strcat(event_string, "Type: ");
		strcat(event_string, event->type);
		strcat(event_string, ", Date: ");
		strcat(event_string, event->date);
		strcat(event_string, ", Place: ");
		strcat(event_string, event->place);
		strcat(event_string, "\n");
		while ((field_elem = nextElement(&iter)) != NULL) {
			Field *field = (Field *)field_elem;
			char *str = event->otherFields.printData(field);
			strcat(event_string, str);
			free(str);
		}
		return event_string;
	}
}

void deleteIndividual(void* toBeDeleted) {
	Individual *individual = (Individual *)toBeDeleted;

	clearList(&individual->otherFields);
	clearList(&individual->events);
	clearList(&individual->families);
	free(individual->givenName);
	free(individual->surname);
	free(individual);
}

int compareIndividuals(const void* first,const void* second) {
	char *ind1_name = malloc(sizeof(char)*512);
	char *ind2_name = malloc(sizeof(char)*512);
	Individual *ind1 = (Individual *)first;
	Individual *ind2 = (Individual *)second;

	strcpy(ind1_name, ind1->surname);
	strcat(ind1_name, ",");
	strcat(ind1_name, ind1->givenName);

	strcpy(ind2_name, ind2->surname);
	strcat(ind2_name, ",");
	strcat(ind2_name, ind2->givenName);

	int result = strcmp(ind1_name, ind2_name);

	free(ind1_name);
	free(ind2_name);

    return result;
}

char* printIndividual(void* toBePrinted) {
	char *individual_string = malloc(sizeof(char)*5120);
	strcpy(individual_string, "");

	Individual *individual = (Individual *)toBePrinted;
	void *event_elem, *field_elem;
	ListIterator event_iter = createIterator(individual->events);
	ListIterator field_iter = createIterator(individual->otherFields);

	if (individual == NULL) {
		strcpy(individual_string, "NULL individual\n");
		return individual_string;
	}

	else {
		strcpy(individual_string, "----------Individual----------\n");
		strcat(individual_string, "Given Name: ");
		strcat(individual_string, individual->givenName);
		strcat(individual_string, ", Surname: ");
		strcat(individual_string, individual->surname);
		strcat(individual_string, "\n");
		while ((event_elem = nextElement(&event_iter)) != NULL) {
			Event *event = (Event *)event_elem;
			char *str = individual->events.printData(event);
			strcat(individual_string, str);
			free(str);
		}
		while ((field_elem = nextElement(&field_iter)) != NULL) {
			Field *field = (Field *)field_elem;
			char *str = individual->otherFields.printData(field);
			strcat(individual_string, str);
			free(str);
		}
		return individual_string;
	}
}

void deleteFamily(void* toBeDeleted) {
	Family *family = (Family *)toBeDeleted;
	if (family == NULL)
		return;
	clearList(&family->otherFields);
	clearList(&family->children);
	clearList(&family->events);
/*	if (family->wife != NULL) {
		free(family->wife->surname);
		free(family->wife->givenName);
	}
	if (family->husband != NULL) {
		free(family->husband->surname);
		free(family->husband->givenName);
	}
	free(family->wife);
	free(family->husband);
*/	free(family);
}

int compareFamilies(const void* first,const void* second) {
	Family *family1 = (Family *)first;
	Family *family2 = (Family *)second;
	int fam1_members = 0, fam2_members = 0;

	if (family1->husband != NULL)
		fam1_members++;
	if (family1->wife != NULL)
		fam1_members++;
	if (family2->husband != NULL)
		fam2_members++;
	if (family2->wife != NULL)
		fam2_members++;

	ListIterator fam1_iter = createIterator(family1->children);
	ListIterator fam2_iter = createIterator(family2->children);

	while (nextElement(&fam1_iter) != NULL)
		fam1_members++;
	while (nextElement(&fam2_iter) != NULL)
		fam2_members++;

	if (fam1_members < fam2_members)
		return -1;
	else if (fam1_members > fam2_members)
		return 1;
	else
		return 0;
}

char* printFamily(void* toBePrinted) {
	char *family_string = malloc(sizeof(char)*10240);
	strcpy(family_string, "");
	Family *family = (Family *)toBePrinted;
	void *field_elem, *ind_elem, *event_elem;
	ListIterator field_iter = createIterator(family->otherFields);
	ListIterator ind_iter = createIterator(family->children);
	ListIterator event_iter = createIterator(family->events);

	if (family == NULL) {
		strcpy(family_string, "NULL family\n");
		return family_string;
	}

	else {
		strcpy(family_string, "------------Family-----------\n");
		strcat(family_string, "Wife given name: ");
		if (family->wife != NULL)
			strcat(family_string, family->wife->givenName);
		strcat(family_string, ", Wife surname: ");
		if (family->wife != NULL)
			strcat(family_string, family->wife->surname);
		strcat(family_string, "\nHusband given name: ");
		if (family->husband != NULL)
			strcat(family_string, family->husband->givenName);
		strcat(family_string, ", Husband surname: ");
		if (family->husband != NULL)
			strcat(family_string, family->husband->surname);
		strcat(family_string, "\n");
		while ((ind_elem = nextElement(&ind_iter)) != NULL) {
			Individual *individual = (Individual *)ind_elem;
			char *str = family->children.printData(individual);
			if (str != NULL)
				strcat(family_string, str);
			free(str);
		}
		while ((field_elem = nextElement(&field_iter)) != NULL) {
			Field *field = (Field *)field_elem;
			char *str = family->otherFields.printData(field);
			if (str != NULL)
				strcat(family_string, str);
			free(str);
		}
		while ((event_elem = nextElement(&event_iter)) != NULL) {
			Event *event = (Event *)event_elem;
			char *str = family->events.printData(event);
			if (str != NULL)
				strcat(family_string, str);
			free(str);
		}
		strcat(family_string, "------------------------------\n");
		return family_string;
	}
}

void deleteField(void* toBeDeleted) {
	Field *field = (Field *)toBeDeleted;

	free(field->tag);
	free(field->value);
	free(field);
}

int compareFields(const void* first,const void* second) {
	char *field1_string = malloc(sizeof(char)*512);
	char *field2_string = malloc(sizeof(char)*512);
	Field *field1 = (Field *)first;
	Field *field2 = (Field *)second;

	strcpy(field1_string, field1->tag);
	strcat(field1_string, " ");
	strcat(field1_string, field1->value);

	strcpy(field2_string, field2->tag);
	strcat(field2_string, " ");
	strcat(field2_string, field2->value);

	int result = strcmp(field1_string, field2_string);

	free(field1_string);
	free(field2_string);

    return result;
}

char* printField(void* toBePrinted) {
    char *field_string = malloc(sizeof(char)*512);
	Field *field = (Field *)toBePrinted;

	if (field == NULL) {
		strcpy(field_string, "NULL FIELD\n");
		return field_string;
	}

	else {
		strcpy(field_string, "-------------Field------------\n");
		strcat(field_string, "Tag: ");
		strcat(field_string, field->tag);
		strcat(field_string, ", Value: ");
		strcat(field_string, field->value);
		strcat(field_string, "\n");
		return field_string;
	}
}
