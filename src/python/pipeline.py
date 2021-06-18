from sys import exit

import directory2datasets

class Node:
    def __init__(self, id, data):
        self.id = id
        self.module = data['module']
        self.parent_id = data['parent']  
        self.children = []
        self.data = data
            
class Graph:
    def __init__(self):
        self.root = None
        self.is_linked = False
        self.dataset = None
        self.is_dataset_defined = False
        self.nodes = {}
        self.node_stack = []

    """
    Prints graph information
    """
    def __repr__(self):
        s = ''
        if self.is_linked:
            s += 'Status: linked\n'
        else:
            s += 'Status: unlinked\n'

        for node_id in self.nodes:
            node = self.nodes[node_id]
            s += '\n%s (%s):\n' % (node.id, node.module)

            s += '\tChildren - %s\n\tData - %s\n' % (node.children, node.data)

        return s

    """
    Prints graph diagram
    """
    def __str__(self):
        if self.is_linked:
            s = 'Pipeline Diagram:\n\n'
            s += self._print_node(self.root, [], True)
            return s
        else:
            print('warning: cannot print graph diagram because nodes have not been linked')
            return self.__repr__()


    """
    Helper method for recursively printing nodes in a tree
    """
    def _print_node(self, node, levels, is_last):
        s = ''

        # gaps and bridges
        for l in levels:
            s += '\t' + '│' * l

        # T or end
        if levels:
            if is_last:
                s += '└─ '
            else:
                s += '├─ '

        # print node
        s += '%s (%s)\n' % (node.id, node.module)

        # recurse for child nodes
        if node.children:
            if not is_last:
                levels[-1] = 1

            levels.append(0)

            n = len(node.children)-1
            for i,child_node in enumerate(node.children):
                s += self._print_node(child_node, levels[:], (i == n))

        return s
    
    """
    Add an unlinked node to the graph
    """
    def add_node(self, node_id, node_data):
        node = Node(node_id, node_data)

        if node_id == 'datasets':
            if self.root is not None:
                print('warning: multiple root nodes exist')
            self.root = node

        self.nodes[node_id] = node

    """
    Link nodes in graph by specifying the children
    """
    def link_nodes(self):
        if self.root == None:
            exit('error: cannot create linkage because there is no root node')

        for node_id in self.nodes:
            node = self.nodes[node_id]
            if node.parent_id is not None:
                self.nodes[node.parent_id].children.append(node)
        
        self.is_linked = True

    """
    Builds a dataset from the pipeline root path
    """
    def build_dataset(self, dryrun=False, verbose=False):
        if not self.is_linked:
            exit('error: pipeline graph must be linked before building the dataset')

        self.datasets = directory2datasets.convert(self.root.data['path'], dryrun, verbose)
        self.is_dataset_defined = True
        
    """
    Executes the pipeline for the defined root node datasets
    """
    def execute(self, dryrun=False, verbose=False):
        if not self.is_dataset_defined:
            exit('error: cannot execute pipeline before building the dataset')
        
        # traverse graph and submit jobs
        self.traverse_graph(self.root, dryrun, verbose)
       
        # TODO: needs to output a processed.json to keep track of what files have been run through the pipeline and what parameters were used

    """
    Recursively traverses graph and submit jobs
    """
    def traverse_graph(self, node, dryrun=False, verbose=False):
        # submit jobs for current node
        self.submit_jobs(node, dryrun, verbose)

        # add child nodes to stack
        for child_node in node.children:
            self.node_stack.append(child_node)

        # recurse if stack not empty
        if self.node_stack:
            self.traverse_graph(self.node_stack.pop(), dryrun, verbose)

    """
    Submit jobs for current node
    """
    def submit_jobs(self, node, dryrun=False, verbose=False):
        # TODO: This is where I stopped. This function should submit a job for all datasets. The plan was to use dependencies within bsub to handle dependencies between nodes.
        pass
